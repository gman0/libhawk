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

#include <exception>
#include <utility>
#include <algorithm>

#include <boost/functional/hash.hpp>
// ^ this resolves an undefined reference to boost::hash_range<>

#include "handlers/dir.h"
#include "handlers/dir_hash.h"

#include "Tab.h"
#include "Column.h"

using namespace boost::filesystem;

constexpr int cache_threshold = 1024;

namespace hawk {

List_dir::List_dir(const boost::filesystem::path& path,
	Column* parent_column)
	:
	Handler{path, parent_column, get_handler_hash<List_dir>()},
	m_path_hash{}
{
	set_path(path);
}

List_dir& List_dir::operator=(List_dir&& ld)
{
	Handler::operator=(std::move(ld));
	m_path_hash = ld.m_path_hash;
	m_dir_items = std::move(ld.m_dir_items);
	m_cursor = std::move(ld.m_cursor);

	return *this;
}

bool List_dir::read_directory()
{
	m_dir_items.clear();

	// free up some space if we need to
	if (m_dir_items.size() > cache_threshold)
		m_dir_items.resize(cache_threshold);

	// gather the directory contents and move them to the vector
	directory_iterator dir_it_end;
	for (auto dir_it = directory_iterator {*m_path};
		 dir_it != dir_it_end;
		 ++dir_it)
	{
		Dir_entry dir_ent;
		dir_ent.path = std::move(*dir_it);
		dir_ent.status = status(dir_ent.path);

		m_dir_items.push_back(std::move(dir_ent));
	}

	// set the cursor

	const path* child_path = m_parent_column->get_child_path();

	// do we have a child path (i.e. child column)?
	if (child_path)
	{
		set_cursor(*child_path);
		return false;
	}
	else
	{
		// try to retrieve the last used cursor

		Tab* tab = m_parent_column->get_parent_tab();
		List_dir::Cursor_map::iterator cursor_hash_it;

		if (tab->find_cursor(m_path_hash, cursor_hash_it))
		{
			// ok, so we DID find the hash of the cursor,
			// now let's assign it to the correct Dir_vector
			// item if we can
			size_t cursor_hash = cursor_hash_it->second;

			Dir_cursor cursor =
				std::find_if(m_dir_items.begin(), m_dir_items.end(),
					[cursor_hash](const Dir_entry& item)
					{ return (hash_value(item.path) == cursor_hash); }
				);

			if (cursor != m_dir_items.end())
			{
				m_cursor = cursor;
				return false;
			}
		}

		// we didn't find the cursor, let's use the first
		// item of our Dir_vector as the cursor (or the
		// end() iterator if the vector is empty)
		m_cursor = m_dir_items.begin();
	}

	return true;
}

void List_dir::set_cursor(List_dir::Dir_cursor cursor)
{
	m_cursor = std::move(cursor);
	size_t cursor_hash;

	if (m_dir_items.empty())
		cursor_hash = 0;
	else
		cursor_hash = hash_value(m_cursor->path);

	m_parent_column->get_parent_tab()->store_cursor(m_path_hash, cursor_hash);

	m_implicit_cursor = false;
}

void List_dir::set_cursor(const path& cur)
{
	size_t cursor_hash = hash_value(cur);

	Dir_cursor cursor =
		std::find_if(m_dir_items.begin(), m_dir_items.end(),
			[&cursor_hash] (const Dir_entry& dir_ent)
			{
				return (hash_value(dir_ent.path) == cursor_hash);
			});

	if (cursor != m_dir_items.end())
		set_cursor(cursor);
	else
		set_cursor(m_dir_items.begin());
}

void List_dir::set_path(const path& dir)
{
	Handler::set_path(dir);

	if (dir.empty())
	{
		m_dir_items.clear();
		m_path_hash = 0;

		return;
	}

	if (!is_directory(dir))
	{
		throw std::runtime_error
			{ std::string {"\""} + dir.c_str() + "\" is not a directory" };
	}

	m_path_hash = hash_value(dir);

	m_implicit_cursor = read_directory();
}

} // namespace hawk
