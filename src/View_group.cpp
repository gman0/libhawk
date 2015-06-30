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
#include "Dir_cache.h"
#include "Interruptible_thread.h"
#include "Interrupt.h"
#include "Filesystem.h"

namespace hawk {

namespace {

struct Global_guard
{
	std::atomic<bool>& global;
	Global_guard(std::atomic<bool>& glob) : global{glob}
	{ global.store(true, std::memory_order_release); }
	~Global_guard()
	{ global.store(false, std::memory_order_release); }
};

std::vector<Path> dissect_path(Path& p, int nviews)
{
	std::vector<Path> paths(nviews + 1);

	for (; nviews >= 0; nviews--)
	{
		paths[nviews] = p;
		p.set_parent_path();
	}

	return paths;
}

int count_empty_views(const View_group::List_dir_vector& v)
{
	return std::count_if(v.begin(), v.end(), [](const auto& c) {
		return c->get_path().empty();
	});
}

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

View_group::~View_group()
{
	if (!m_tasking_thread.joinable())
		return;

	m_tasking_thread.soft_interrupt();

	std::unique_lock<std::mutex> lk {m_tasking.m};
	m_tasking_thread.hard_interrupt();
	m_tasking.run_task(lk, []{
		for (;;)
			hard_interruption_point();
	});

	m_tasking_thread.join();
}

Path View_group::get_path() const
{
	Path p;

	{
		std::shared_lock<std::shared_timed_mutex> lk {m_path_sm};
		p = m_path;
	}

	return p;
}

void View_group::task_load_path(const Path& p)
{
	Global_guard gg {m_tasking.global};

	update_paths(p);
	soft_interruption_point();

	update_active_cursor();
	destroy_free_dir_ptrs(count_empty_views(m_views) + 1);
}

void View_group::set_path(Path p)
{
	try { p = canonical(p, m_path); }
	catch (...) { m_tasking.exception_handler(std::current_exception()); }

	if (p.empty())
		p = "/";
	else
		check_and_rollback_path(p, m_path);

	m_tasking_thread.soft_interrupt();

	std::unique_lock<std::mutex> lk {m_tasking.m};
	m_tasking.run_task(lk, [&, p]{
		task_load_path(p);

		std::lock_guard<std::shared_timed_mutex> lk {m_path_sm};
		m_path = p;
	});
}

void View_group::reload_path()
{
	std::unique_lock<std::shared_timed_mutex> lk_path {m_path_sm};
	check_and_rollback_path(m_path, m_path);

	m_tasking_thread.soft_interrupt();
	std::unique_lock<std::mutex> lk {m_tasking.m};
	m_tasking.run_task(lk, [this, l {std::move(lk_path)}]{
		task_load_path(m_path);
	});
}

const View_group::List_dir_vector& View_group::get_views() const
{
	return m_views;
}

void View_group::set_cursor(Dir_cursor cursor)
{
	if (can_set_cursor())
	{
		List_dir_ptr& ld = m_views.back();
		ld->set_cursor(cursor);
		create_preview({ld->get_path() / cursor->path});
	}
}

void View_group::set_cursor(const Path& filename,
					 List_dir::Cursor_search_direction dir)
{
	if (can_set_cursor())
	{
		List_dir_ptr& ld = m_views.back();
		ld->set_cursor(filename, dir);
		create_preview({ld->get_path() / ld->get_cursor()->path});
	}
}

void View_group::build_views(int nviews)
{
	instantiate_views(nviews);
	initialize_views(nviews);

	update_active_cursor();
}

void View_group::instantiate_views(int nviews)
{
	add_view(m_list_dir_closure);
	for (int i = 1; i <= nviews; i++)
	{
		add_view(m_list_dir_closure);
		m_views[i - 1]->_set_next_view(m_views[i].get());
	}
}

void View_group::initialize_views(int nviews)
{
	Path p = m_path;
	std::vector<Path> paths = dissect_path(p, nviews);

	for (; nviews >= 0; nviews--)
		ready_view(*m_views[nviews], paths[nviews]);
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

void View_group::add_view(const Type_factory::Handler& closure)
{
	m_views.emplace_back(static_cast<List_dir*>(closure()));
}

void View_group::ready_view(View& v, const Path& p)
{
	v.set_path(p);
	v.ready();
}

void View_group::update_active_cursor()
{
	if (can_set_cursor())
	{
		List_dir_ptr& ld = m_views.back();
		task_create_preview({ld->get_path() / ld->get_cursor()->path});
	}
}

bool View_group::can_set_cursor()
{
	destroy_preview();
	return !m_views.back()->get_contents().empty();
}

void View_group::create_preview(const Path& p)
{
	std::unique_lock<std::mutex> lk {m_tasking.m};
	if (m_tasking.global.load(std::memory_order_acquire))
		return;

	m_tasking_thread.soft_interrupt();
	m_tasking.run_task(lk, [&, p]{
		// If the user is scrolling the cursor too fast, don't
		// create the preview immediately. Instead, wait for
		// m_preview_delay milliseconds whilst checking for
		// interrupts.
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
		task_create_preview(p);
	});
}

void View_group::task_create_preview(const Path& p)
{
	auto handler = m_type_factory.get_handler(p);
	if (!handler) return;

	m_preview.reset(handler());
	soft_interruption_point();

	try { ready_view(*m_preview, p); }
	catch (...)
	{
		destroy_preview();
		throw;
	}
}

void View_group::destroy_preview()
{
	m_preview.reset();
}

void View_group::Tasking::run_task(std::unique_lock<std::mutex>& lk,
							std::function<void()>&& f)
{
	cv.wait(lk, [this]{ return ready_for_tasking; });

	ready_for_tasking = false;
	task = std::move(f);

	lk.unlock();
	cv.notify_one();
}

void View_group::Tasking::operator()()
{
	std::unique_lock<std::mutex> lk {m, std::defer_lock};

	auto start_task = [&]{
		lk.lock();
		cv.wait(lk, [this]{ return !ready_for_tasking; });
		_soft_iflag.clear();
		lk.unlock();
		cv.notify_one();
	};

	auto end_task = [&]{
		lk.lock();
		ready_for_tasking = true;
		lk.unlock();
		cv.notify_one();
	};

	for (;;)
	{
		start_task();

		try { task(); }
		catch (const Soft_thread_interrupt&) {}
		catch (const Hard_thread_interrupt&) { throw; }
		catch (...) {
			exception_handler(std::current_exception());
		}

		end_task();
	}
}

} // namespace hawk
