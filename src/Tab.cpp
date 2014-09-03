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
#include <algorithm>
#include <utility>
#include <exception>
#include "Tab.h"
#include "Cursor_cache.h"
#include "Column.h"

using namespace boost::filesystem;

namespace hawk {

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
	static struct
	{
		path prev_path;
		char fail_level = 0;
		std::exception_ptr eptr;
	} err;

	try
	{
		p = canonical(p, m_path);
		if (p.empty())
			p = "/";

		update_paths(p);
	}
	catch (...)
	{
		if (err.fail_level == 0)
		{
			++err.fail_level;
			err.prev_path = std::move(m_path);
			err.eptr = std::current_exception();

			set_path(err.prev_path);
		}
		else
		{
			err.fail_level = 2;
			err.prev_path = err.prev_path.parent_path();

			set_path(err.prev_path);
		}
	}

	update_active_cursor();
	m_path = std::move(p);

	if (err.fail_level)
	{
		err.fail_level = 0;
		std::rethrow_exception(err.eptr);
	}
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
	for (int i = 1; i < ncols; i++)
	{
		add_column(m_list_dir_closure);
		m_columns[i - 1]->_set_next_column(m_columns[i].get());
	}
}

void Tab::initialize_columns(int ncols)
{
	static std::vector<path> path_vec;
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

	m_columns[ncols]->set_path(p);
	for (int i = ncols - 1; i >= 0; i--)
	{
		p = p.parent_path();
		m_columns[i]->set_path(p);
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
