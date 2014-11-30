/*
	Copyright (C) 2013-2014 Róbert "gman" Vašek <gman@codefreax.org>

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
#include "Tab.h"
#include "Column.h"
#include "handlers/List_dir.h"
#include "Dir_cache.h"
#include "Interruptible_thread.h"
#include "Interrupt.h"
#include "Filesystem.h"

namespace hawk {

struct Global_guard
{
	std::atomic<bool>& global;
	Global_guard(std::atomic<bool>& glob) : global{glob}
	{ global.store(true, std::memory_order_release); }
	~Global_guard()
	{ global.store(false, std::memory_order_release); }
};

static std::vector<Path> dissect_path(Path& p, int ncols)
{
	std::vector<Path> paths(ncols + 1);

	for (; ncols >= 0; ncols--)
	{
		paths[ncols] = p;
		p = p.parent_path();
	}

	return paths;
}

static int get_empty_columns(const Tab::List_dir_vector& ld_vec)
{
	int n = 0;
	for (auto& col : ld_vec)
	{
		if (col->get_path().empty())
			++n;
	}

	return n;
}

// Checks path for validity
static void check_and_rollback_path(Path& p, const Path& old_p)
{
	if (directory_readable(p)) return; // We're good to go.

	p = old_p; // Roll back to the previous path.
	if (directory_readable(p)) return;

	// Otherwise roll back to the parent directory until it's readable.
	while (!directory_readable(p))
		p = p.parent_path();

	if (p.empty())
		p = "/";
}

Tab::~Tab()
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

Path Tab::get_path() const
{
	Path p;

	{
		std::shared_lock<std::shared_timed_mutex> lk {m_path_sm};
		p = m_path;
	}

	return m_path;
}

void Tab::task_set_path(const Path& p)
{
	Global_guard gg {m_tasking.global};

	update_paths(p);
	soft_interruption_point();

	update_active_cursor();
	destroy_free_dir_ptrs(get_empty_columns(m_columns) + 1);
	soft_interruption_point();

	{
		std::lock_guard<std::shared_timed_mutex> lk {m_path_sm};
		m_path = p;
	}
}

void Tab::set_path(Path p)
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
		task_set_path(p);
	});
}

const Tab::List_dir_vector& Tab::get_columns() const
{
	return m_columns;
}

void Tab::set_cursor(Dir_cursor cursor)
{
	if (can_set_cursor())
	{
		List_dir_ptr& ld = m_columns.back();
		ld->set_cursor(cursor);
		create_preview({ld->get_path() / cursor->path});
	}
}

void Tab::set_cursor(const Path& filename)
{
	if (can_set_cursor())
	{
		List_dir_ptr& ld = m_columns.back();
		ld->set_cursor(filename);
		create_preview({ld->get_path() / filename});
	}
}

void Tab::build_columns(int ncols)
{
	instantiate_columns(ncols);
	initialize_columns(ncols);

	update_active_cursor();
}

void Tab::instantiate_columns(int ncols)
{
	add_column(m_list_dir_closure);
	for (int i = 1; i <= ncols; i++)
	{
		add_column(m_list_dir_closure);
		m_columns[i - 1]->_set_next_column(m_columns[i].get());
	}
}

void Tab::initialize_columns(int ncols)
{
	Path p = m_path;
	std::vector<Path> paths = dissect_path(p, ncols);

	for (; ncols >= 0; ncols--)
		ready_column(*m_columns[ncols], paths[ncols]);
}

void Tab::update_paths(Path p)
{
	int ncols = m_columns.size() - 1;

	ready_column(*m_columns[ncols], p);
	for (int i = ncols - 1; i >= 0; i--)
	{
		p = p.parent_path();
		ready_column(*m_columns[i], p);
	}
}

void Tab::add_column(const Type_factory::Handler& closure)
{
	m_columns.emplace_back(static_cast<List_dir*>(closure()));
}

void Tab::ready_column(Column& col, const Path& p)
{
	col.set_path(p);
	col.ready();
}

void Tab::update_active_cursor()
{
	if (can_set_cursor())
	{
		List_dir_ptr& ld = m_columns.back();
		task_create_preview({ld->get_path() / ld->get_cursor()->path});
	}
}

bool Tab::can_set_cursor()
{
	destroy_preview();
	return !m_columns.back()->get_contents().empty();
}

void Tab::create_preview(const Path& p)
{
	std::unique_lock<std::mutex> lk {m_tasking.m};
	if (m_tasking.global.load(std::memory_order_acquire))
		return;

	m_tasking_thread.soft_interrupt();
	m_tasking.run_task(lk, [&, p]{
		// If the user is scrolling the cursor too fast, don't
		// create the preview immediately. Instead, wait
		// m_preview_delay milliseconds, while checking for
		// interrupts.
		auto now = std::chrono::steady_clock::now();
		if (m_preview_delay + m_preview_timestamp > now)
		{
			for (auto t = m_preview_delay; t.count() != 0; --t)
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

void Tab::task_create_preview(const Path& p)
{
	auto handler = m_type_factory->get_handler(p);
	if (!handler) return;

	m_preview.reset(handler());
	soft_interruption_point();

	try { ready_column(*m_preview, p); }
	catch (...)
	{
		destroy_preview();
		throw;
	}
}

void Tab::destroy_preview()
{
	m_preview.reset();
}

void Tab::Tasking::run_task(std::unique_lock<std::mutex>& lk,
							std::function<void()>&& f)
{
	cv.wait(lk, [this]{ return ready_for_tasking; });

	ready_for_tasking = false;
	task = std::move(f);

	lk.unlock();
	cv.notify_one();
}

void Tab::Tasking::operator()()
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
