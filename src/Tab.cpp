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
	if (p.empty()) return;

	p = canonical(p, m_path);

	if (p.empty())
		p = "/";

	update_paths(p);
	update_active_cursor();

	m_path = std::move(p);
}

void Tab::set_path(const path& p,
	boost::system::error_code& ec) noexcept
{
	try {
		set_path(p);
	}
	catch (const boost::filesystem::filesystem_error& e)
	{
		ec = e.code();
	}
}

Tab::Column_vector& Tab::get_columns()
{
	return m_columns;
}

const Tab::Column_vector& Tab::get_columns() const
{
	return m_columns;
}

int Tab::get_current_ncols()
{
	int ncols = m_columns.size();
	return (m_has_preview ? (ncols - 2) : --ncols);
}

List_dir::Dir_cursor Tab::get_begin_cursor() const
{
	List_dir* ld =
		static_cast<List_dir*>(m_active_column);
	return ld->get_contents().begin();
}

void Tab::set_cursor(List_dir::Dir_cursor cursor)
{
	// remove current preview column (if any)
	if (m_has_preview)
	{
		m_columns.pop_back();
		m_has_preview = false;
	}

	// reset the active column
	activate_last_column();

	List_dir* ld = get_active_ld();

	if (ld->empty())
		return;

	// set the cursor in the active column
	ld->set_cursor(cursor);

	// add new preview column if we need to

	Type_factory::Handler closure = m_type_factory->get_handler(cursor->path);

	if (closure)
	{
		add_column(cursor->path, closure);
		m_has_preview = true;
	}
}

void Tab::set_cursor(List_dir::Dir_cursor cursor,
	boost::system::error_code& ec) noexcept
{
	try
	{
		set_cursor(cursor);
	}
	catch (const filesystem_error& e)
	{
		ec = e.code();
		m_columns.erase(--m_columns.end());
		m_has_preview = false;
	}
}

void Tab::build_columns(int ncols)
{
	static std::vector<path> path_vec;
	path_vec.clear();

	path p = m_path;
	dissect_path(p, ncols, path_vec);

	for (int i = 0; i < ncols; i++)
	{
		add_column(std::move(path_vec[i]));
	}

	auto it = m_columns.cbegin();
	for (int i = 0; i < ncols; i++)
	{
		m_columns[i]->_set_next_column( (++it)->get() );
	}

	activate_last_column();
	update_active_cursor();
}

void Tab::update_paths(path p)
{
	int ncols = get_current_ncols();

	m_columns[ncols]->set_path(p);
	for (int i = ncols - 1; i >= 0; i--)
	{
		p = std::move(p.parent_path());
		m_columns[i]->set_path(p);
	}
}

void Tab::remove_column()
{
	// let's leave at least 1 column
	if (m_columns.size() == 1)
		return;

	m_columns.erase(m_columns.begin());
}

void Tab::add_column(const path& p)
{
	add_column(p, m_list_dir_closure);
}

void Tab::add_column(path&& p)
{
	add_column(std::move(p), m_list_dir_closure);
}

void Tab::add_column(const path& p,
	const Type_factory::Handler& closure)
{
	m_columns.emplace_back(closure(p));
}

void Tab::add_column(path&& p,
	const Type_factory::Handler& closure)
{
	m_columns.emplace_back(closure(p));
}

void Tab::add_column(const path& p,
	const Type_factory::Handler& closure, unsigned inplace_col)
{
	m_columns[inplace_col].reset(closure(p));
}

void Tab::update_active_cursor()
{
	List_dir* active_handler =
		static_cast<List_dir*>(m_active_column);

	set_cursor(active_handler->get_cursor());
}

const boost::filesystem::path* Tab::get_last_column_path() const
{
	return &(m_columns.back()->get_path());
}

} // namespace hawk
