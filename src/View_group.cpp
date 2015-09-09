/*
	Copyright (C) 2013-2015 Róbert "gman" Vašek <gman@codefreax.org>

	This file is part of libhawk.

	libhawk is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	libhawk is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with libhawk.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <utility>
#include <exception>
#include <chrono>
#include <utility>
#include "View_group.h"
#include "View.h"
#include "handlers/List_dir.h"
#include "handlers/List_dir_hash.h"
#include "Cursor_cache.h"
#include "Interruptible_thread.h"
#include "Interrupt.h"
#include "Filesystem.h"

namespace hawk {

namespace {

// Checks path for validity
void check_and_rollback_path(Path& p, const Path& old_p)
{
	if (is_readable(p)) return; // We're good to go.

	p = old_p; // Roll back to the previous path.
	if (is_readable(p)) return;

	// Otherwise roll back to the parent directory until it's readable.
	while (!is_readable(p))
		p.set_parent_path();

	if (p.empty())
		p = "/";
}

} // unnamed-namespace

Path View_group::get_path() const
{
	std::shared_lock<std::shared_timed_mutex> lk {m_path_sm};
	return m_path;
}

void View_group::set_path(Path p)
{
	try { p = canonical(p, m_path); }
	catch (...) { m_exception_handler(std::current_exception()); }

	if (p.empty())
		p = "/";
	else
		check_and_rollback_path(p, get_path());

	m_tasking.run({
		{Tasking::Priority::high, [&, p]{
			update_cursors(p);
			update_paths(p);

			std::lock_guard<std::shared_timed_mutex> lk {m_path_sm};
			m_path = p;
		 }
		},
		{Tasking::Priority::low, [this]{
			update_active_cursor();
		 }
		}
	});
}

void View_group::reload_path()
{
	std::unique_lock<std::shared_timed_mutex> lk_path {m_path_sm};
	check_and_rollback_path(m_path, m_path);

	m_tasking.run({
		{Tasking::Priority::high, [this, l = {std::move(lk_path)}]{
			 update_cursors(m_path);
			 update_paths(m_path);
		 }
		},
		{Tasking::Priority::low, [this]{
			 update_active_cursor();
		 }
		}
	});
}

const View_group::List_dir_vector& View_group::get_views() const
{
	return m_views;
}

const View* View_group::get_preview() const
{
	return m_preview.get();
}

Path View_group::get_cursor_path() const
{
	std::shared_lock<std::shared_timed_mutex> lk {m_preview_path_sm};
	return m_preview_path;
}

void View_group::set_cursor(const Path& filename,
							List_dir::Cursor_search_direction dir)
{
	m_tasking.run_blocking(Tasking::Priority::low, [&]{
		if (can_set_cursor())
		{
			List_dir_ptr& ld = m_views.back();
			ld->set_cursor(filename, dir);
			set_cursor_path(m_path / ld->get_cursor()->path);
		}
	});

	delay_create_preview();
}

void View_group::advance_cursor(Dir_vector::difference_type d)
{
	m_tasking.run_blocking(Tasking::Priority::low, [&]{
		if (can_set_cursor())
		{
			List_dir_ptr& ld = m_views.back();
			ld->advance_cursor(d);
			set_cursor_path(m_path / ld->get_cursor()->path);
		}
	});

	delay_create_preview();
}

void View_group::rewind_cursor(List_dir::Cursor_position p)
{
	m_tasking.run_blocking(Tasking::Priority::low, [&]{
		if (can_set_cursor())
		{
			List_dir_ptr& ld = m_views.back();
			ld->rewind_cursor(p);
			set_cursor_path(m_path / ld->get_cursor()->path);
		}
	});

	delay_create_preview();
}

void View_group::update_cursors(Path path)
{
	set_cursor_path("");

	Path filename = path.filename();
	path.set_parent_path();

	while (!filename.empty() && !path.empty())
	{
		m_cursor_cache.store(path.hash(), filename.hash());

		filename = path.filename();
		path.set_parent_path();
	}
}

void View_group::set_cursor_path(const Path& p)
{
	std::lock_guard<std::shared_timed_mutex> lk {m_preview_path_sm};
	m_preview_path = p;
}

void View_group::update_paths(Path p)
{
	int nviews = m_views.size() - 1;

	ready_view(*m_views[nviews], p);
	for (int i = nviews - 1; i >= 0; i--)
	{
		p.set_parent_path();
		ready_view(*m_views[i], p);
	}
}

void View_group::add_view(const View_types::Handler& list_dir)
{
	m_tasking.run_noint_blocking(Tasking::Priority::high, [&]{
		m_views.emplace_back(static_cast<List_dir*>(list_dir(*this)));
	});
}

void View_group::remove_view()
{
	m_tasking.run_noint_blocking(Tasking::Priority::high, [&]{
		m_views.erase(m_views.begin());
	});
}

void View_group::ready_view(View& v, const Path& p)
{
	View::Ready_guard rg {v};
	v.set_path(p);
}

void View_group::update_active_cursor()
{
	set_cursor_path("");

	if (can_set_cursor())
	{
		List_dir_ptr& ld = m_views.back();
		create_preview({ld->get_path() / ld->get_cursor()->path}, true);
	}
}

bool View_group::can_set_cursor()
{
	destroy_preview();
	const Dir_vector* dir = m_views.back()->get_contents();

	return dir && !dir->empty();
}

void View_group::create_preview(const Path& p, bool set_cpath)
{
	if (set_cpath)
		set_cursor_path(p);

	auto handler = m_view_types.get_handler(p);
	if (!handler) return;

	destroy_preview();
	soft_interruption_point();

	// ready_view() can throw, we need to separate the assignment to m_preview
	View_ptr preview {handler(*this)};
	ready_view(*preview, p);

	m_preview = std::move(preview);
}

void View_group::destroy_preview()
{
	m_preview.reset();
}

void View_group::delay_preview()
{
	auto now = std::chrono::steady_clock::now();
	if (m_preview_delay + m_preview_timestamp > now)
	{
		for (int i = m_preview_delay.count(); i != 0; i--)
		{
			std::this_thread::sleep_for(
						std::chrono::milliseconds{1});
			soft_interruption_point();
		}
	}

	m_preview_timestamp = now;
}

void View_group::delay_create_preview()
{
	m_tasking.run(Tasking::Priority::low, [this]{
		delay_preview();

		List_dir_ptr& ld = m_views.back();
		create_preview(ld->get_path() / ld->get_cursor()->path, false);
	});
}

} // namespace hawk
