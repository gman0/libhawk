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
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include "Tab.h"
#include "Cursor_cache.h"
#include "Column.h"
#include "Dir_cache.h"

using namespace boost::filesystem;

namespace hawk {

static std::mutex s_mutex;
static std::mutex s_path_mutex;
static std::mutex s_cursor_mutex;

static void dissect_path(path& p, int ncols, std::vector<path>& out)
{
	for (int i = 0; i < ncols; i++)
		out.emplace_back();

	--ncols;
	for (; ncols >= 0; ncols--)
	{
		out[ncols] = p;
		p = p.parent_path();
	}
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

Tab::Tab(const path& p, Cursor_cache* cc, int ncols,
		 Type_factory* tf, const Type_factory::Handler& list_dir_closure)
	:
	  m_path{p},
	  m_columns{},
	  m_type_factory{tf},
	  m_list_dir_closure{list_dir_closure},
	  m_has_preview{false},
	  m_cursor_cache{cc}
{
	build_columns(ncols);
}

Tab::Tab(path&& p, Cursor_cache* cc, int ncols,
		 Type_factory* tf, const Type_factory::Handler& list_dir_closure)
	:
	  m_path{std::move(p)},
	  m_columns{},
	  m_type_factory{tf},
	  m_list_dir_closure{list_dir_closure},
	  m_has_preview{false},
	  m_cursor_cache{cc}
{
	build_columns(ncols);
}

const path& Tab::get_path() const
{
	return m_path;
}

void Tab::set_path(path p)
{
	std::exception_ptr eptr;
	try { p = canonical(p, m_path); }
	catch (...) { eptr = std::current_exception(); }

	if (p.empty())
		p = "/";

	{
		std::lock_guard<std::mutex> lk {s_mutex};
		check_and_rollback_path(p, m_path);
		update_paths(p);
		update_active_cursor();
		destroy_free_dir_ptrs(get_empty_columns(m_columns));

		std::lock_guard<std::mutex> lk_path {s_path_mutex};
		m_path = std::move(p);
	}

	if (eptr)
		std::rethrow_exception(eptr);
}

void Tab::reload_current_path()
{
	path p;

	{
		std::unique_lock<std::mutex> lk {s_path_mutex, std::defer_lock};
		for (;;)
		{
			lk.lock();
			p = m_path;
			lk.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds{1});
			lk.lock();
			if (p == m_path) break;
			lk.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds{1});
		}
	}

	set_path(p);
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
	std::lock_guard<std::mutex> lk {s_cursor_mutex};
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
	std::lock_guard<std::mutex> lk {s_cursor_mutex};
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
	for (int i = 1; i < ncols; i++)
	{
		add_column(m_list_dir_closure);
		m_columns[i - 1]->_set_next_column(m_columns[i].get());
	}
}

void Tab::initialize_columns(int ncols)
{
	std::vector<path> path_vec;
	path_vec.clear();

	path p = m_path;
	dissect_path(p, ncols, path_vec);

	--ncols;
	for (; ncols >= 0; ncols--)
		ready_column(m_columns[ncols], path_vec[ncols]);
}

void Tab::update_paths(path p)
{
	int ncols = m_columns.size();

	if (m_has_preview)
		ncols -= 2;
	else
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
	set_cursor(m_active_ld->get_cursor());
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
		m_has_preview = true;
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
	if (m_has_preview)
	{
		m_columns.pop_back();
		m_has_preview = false;
	}
}

} // namespace hawk
