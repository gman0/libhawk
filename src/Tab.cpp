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

#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <utility>
#include <exception>
#include <chrono>
#include "Tab.h"
#include "Column.h"
#include "handlers/List_dir.h"
#include "Dir_cache.h"
#include "Interruptible_thread.h"
#include "Interrupt.h"

using namespace boost::filesystem;

namespace hawk {

class Preview
{
private:
	std::unique_ptr<Column> m_column;
	Type_factory* m_type_factory;

public:
	Preview(Type_factory* tf) : m_type_factory{tf} {}

	void create()
	{

	}

	void destroy()
	{

	}

	void cancel()
	{

	}

	bool has_preview() { return static_cast<bool>(m_column); }
};

struct Global_guard
{
	std::atomic<bool>& global;
	Global_guard(std::atomic<bool>& glob) : global{glob}
	{ global.store(true, std::memory_order_release); }
	~Global_guard()
	{ global.store(false, std::memory_order_release); }
};

static std::vector<path> dissect_path(path& p, int ncols)
{
	std::vector<path> paths(ncols + 1);

	for (; ncols >= 0; ncols--)
	{
		paths[ncols] = p;
		p = p.parent_path();
	}

	return paths;
}

static int get_empty_columns(const Tab::Column_vector& col_vec)
{
	int n = 0;
	for (auto& col : col_vec)
	{
		if (col->get_path().empty())
			++n;
	}

	return n;
}

// Fool-proof check for directory reading privileges.
static bool dir_readable(const path& p)
{
	boost::system::error_code ec;
	if (!is_directory(p, ec)) return false;

	directory_iterator it {p, ec};
	return !ec;
}

// Checks path for validity
static void check_and_rollback_path(path& p, const path& old_p)
{
	if (dir_readable(p)) return; // We're good to go.

	p = old_p; // Roll back to the previous path.
	if (dir_readable(p)) return;

	// Otherwise roll back to the parent directory until it's readable.
	while (!dir_readable(p))
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

const path& Tab::get_path() const
{
	return m_path;
}

void Tab::task_set_path(const path& p)
{
	Global_guard gg {m_tasking.global};

	update_paths(p);
	soft_interruption_point();

	update_active_cursor();
	destroy_free_dir_ptrs(get_empty_columns(m_columns) + 1);
	soft_interruption_point();

	m_path = p;
}

void Tab::set_path(path p)
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

const Tab::Column_vector& Tab::get_columns() const
{
	return m_columns;
}

List_dir* const Tab::get_active_list_dir() const
{
	return m_active_ld;
}

void Tab::set_cursor(Dir_cursor cursor)
{
	if (prepare_cursor())
	{
		// Set the cursor in the active column.
		m_active_ld->set_cursor(cursor);
		// Add a new preview column if we can.
		add_preview(cursor->path);
	}
}

void Tab::set_cursor(const path& p)
{
	if (prepare_cursor())
	{
		// Set the cursor in the active column.
		m_active_ld->set_cursor(p);
		// Add a new preview column if we can.
		add_preview(p);
	}
}

void Tab::build_columns(int ncols)
{
	instantiate_columns(ncols);
	initialize_columns(ncols);

	activate_last_column();
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
	path p = m_path;
	std::vector<path> paths = dissect_path(p, ncols);

	for (; ncols >= 0; ncols--)
		ready_column(m_columns[ncols], paths[ncols]);
}

void Tab::update_paths(path p)
{
	int ncols = m_columns.size();

//	if (m_has_preview)
//		ncols -= 2;
//	else
		--ncols;

	ready_column(m_columns[ncols], p);
	for (int i = ncols - 1; i >= 0; i--)
	{
		p = p.parent_path();
		ready_column(m_columns[i], p);
	}
}

void Tab::add_column(const Type_factory::Handler& closure)
{
	m_columns.emplace_back(closure());
}

void Tab::ready_column(Column_ptr& col, const path& p)
{
	col->set_path(p);
	col->ready();
}

void Tab::update_active_cursor()
{
	//set_cursor(m_active_ld->get_cursor());
}

void Tab::activate_last_column()
{
	m_active_ld = static_cast<List_dir*>(m_columns.back().get());
}

bool Tab::prepare_cursor()
{
	// Remove the preview column if there's one.
	remove_preview();
	// Reset the active column.
	activate_last_column();

	return !m_active_ld->get_contents().empty();
}

void Tab::add_preview(const path& p)
{
	Type_factory::Handler closure = m_type_factory->get_handler(p);

	if (!closure)
		return;

	add_column(closure);

	try
	{
//		m_has_preview = true;
		ready_column(m_columns.back(), p);
	}
	catch (...)
	{
		remove_preview();
		throw;
	}
}

void Tab::remove_preview()
{
	/*
	if (m_has_preview)
	{
		m_columns.pop_back();
		m_has_preview = false;
	}
	*/
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
