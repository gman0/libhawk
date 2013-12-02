/*
	Copyright (C) 2013 Róbert "gman" Vašek <gman@codefreax.org>

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
#include <exception>
#include "Tab.h"
#include "handlers/dir.h"
#include "handlers/dir_hash_extern.h"

using namespace hawk;
using namespace boost::filesystem;

static std::vector<path> parent_paths;

static void generate_parent_paths(std::vector<path>& v,
	path& p, int n)
{
	v.clear(); // we need to start with a clean vector
	v.push_back(p);

	--n;
	for (; n >= 0; --n)
	{
		p = p.parent_path();
		v.push_back(p);
	}
}


Tab::Tab(const path& pwd, unsigned ncols,
	Type_factory* tf, const Type_factory::Type_product& list_dir_closure)
	:
	m_pwd{pwd},
	m_type_factory{tf},
	m_list_dir_closure{list_dir_closure},
	m_has_preview{false}
{
	m_columns.reserve(ncols + 1);
	build_columns(ncols);
}

Tab::Tab(path&& pwd, unsigned ncols,
	Type_factory* tf, const Type_factory::Type_product& list_dir_closure)
	:
	m_pwd{std::move(pwd)},
	m_type_factory{tf},
	m_list_dir_closure{list_dir_closure},
	m_has_preview{false}
{
	m_columns.reserve(ncols + 1);
	build_columns(ncols);
}

Tab::Tab(const Tab& t)
	:
	m_pwd{t.m_pwd},
	m_columns{t.m_columns},
	m_type_factory{t.m_type_factory},
	m_list_dir_closure{t.m_list_dir_closure},
	m_has_preview{t.m_has_preview}
{
	m_active_column = &(m_columns.back());
}

Tab& Tab::operator=(const Tab& t)
{
	m_pwd = t.m_pwd;
	m_columns = t.m_columns;
	m_type_factory = t.m_type_factory;
	m_list_dir_closure = t.m_list_dir_closure;
	m_has_preview = t.m_has_preview;

	return *this;
}

Tab& Tab::operator=(Tab&& t)
{
	m_pwd = std::move(t.m_pwd);
	m_columns = std::move(t.m_columns);
	m_type_factory = t.m_type_factory;
	m_list_dir_closure = std::move(t.m_list_dir_closure);
	m_has_preview = t.m_has_preview;

	return *this;
}

const path& Tab::get_pwd() const
{
	return m_pwd;
}

void Tab::set_pwd(const path& pwd)
{
	m_pwd = pwd;
	update_paths(pwd);
	update_cursor();
}

std::vector<Column>& Tab::get_columns()
{
	return m_columns;
}

const std::vector<Column>& Tab::get_columns() const
{
	return m_columns;
}

void Tab::set_cursor(const List_dir::Dir_cursor& cursor)
{
	// remove current preview column (if any)
	if (m_has_preview)
	{
		m_columns.erase(--m_columns.end());
		m_has_preview = false;
	}


	// reset the active column
	activate_last_column();


	// set the cursor in the active column

	Handler* handler = m_active_column->get_handler();
	if (handler->get_type() != get_handler_hash<List_dir>())
	{
		throw std::logic_error
			{ "Attempt to move List_dir cursor outside of List_dir" };
	}

	static_cast<List_dir*>(handler)->set_cursor(cursor);


	// add new preview column if we need to

	Type_factory::Type_product closure =
		(*m_type_factory)[*cursor];

	if (closure)
	{
		add_column(*cursor, closure);
		m_has_preview = true;
	}
}

void Tab::build_columns(unsigned ncols)
{
	path p = m_pwd;
	generate_parent_paths(parent_paths, p, --ncols);

	std::for_each(parent_paths.rbegin(), parent_paths.rend(),
		[this](path& pwd){ add_column(std::move(pwd), m_list_dir_closure); });

	activate_last_column();
	update_cursor();
}

void Tab::update_paths(path pwd)
{
	size_t ncols = m_columns.size();
	ncols -= (m_has_preview) ? 1 : 0;

	m_columns[--ncols].set_path(pwd);
	for (int i = ncols - 1; i >= 0; i--)
	{
		pwd = pwd.parent_path();
		m_columns[i].set_path(pwd);
	}
}

void Tab::remove_column()
{
	// let's leave at least 1 column
	if (m_columns.size() == 1)
		return;

	m_columns.erase(m_columns.begin());
}

void Tab::add_column(const path& pwd)
{
	add_column(pwd, m_list_dir_closure);
}

void Tab::add_column(const path& pwd,
	const Type_factory::Type_product& closure)
{
	m_columns.push_back({pwd, closure});
}

void Tab::add_column(path&& pwd,
	const Type_factory::Type_product& closure)
{
	m_columns.push_back({std::move(pwd), closure});
}

void Tab::add_column(const path& pwd,
	const Type_factory::Type_product& closure, unsigned inplace_col)
{
	m_columns[inplace_col] = {pwd, closure};
}

void Tab::update_cursor()
{
	List_dir* active_handler =
		static_cast<List_dir*>(m_active_column->get_handler());

	set_cursor(active_handler->get_cursor());
}
