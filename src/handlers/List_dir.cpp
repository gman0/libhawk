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

#include <exception>
#include <utility>
#include <algorithm>

#include <boost/functional/hash.hpp>
// ^ this resolves an undefined reference to boost::hash_range<>

#include "handlers/List_dir.h"
#include "handlers/List_dir_hash.h"

#include "Tab.h"
#include "Cursor_cache.h"
#include "Column.h"

using namespace boost::filesystem;

namespace hawk {

// Gather contents of a directory and move them to the out vector
static void gather_dir_contents(const path& p, List_dir::Dir_vector& out)
{
	directory_iterator dir_it_end;
	for (auto dir_it = directory_iterator {p};
		 dir_it != dir_it_end;
		 ++dir_it)
	{
		List_dir::Dir_entry dir_ent;
		dir_ent.path = std::move(*dir_it);

		boost::system::error_code ec;
		dir_ent.status = status(dir_ent.path, ec);

		out.push_back(std::move(dir_ent));
	}
}

// Checks for absolute paths.
static List_dir::Dir_cursor match_cursor_absolute(List_dir::Dir_vector& vec,
												  size_t match_hash)
{
	return std::find_if(vec.begin(), vec.end(),
				[match_hash](const List_dir::Dir_entry& dir_ent)
				{ return (hash_value(dir_ent.path) == match_hash); });
}

// Checks for filenames only.
static List_dir::Dir_cursor match_cursor_filename(List_dir::Dir_vector& vec,
												 const path& filename)
{
	size_t hash = hash_value(filename);
	const auto predicate =
			[hash](const List_dir::Dir_entry& ent)
			{ return (hash_value(ent.path.filename())) == hash; };

	return std::find_if(vec.begin(), vec.end(), predicate);
}

List_dir::List_dir(List_dir&& ld) noexcept
	:
	  Column{std::move(ld)},
	  m_cursor_cache{ld.m_cursor_cache},
	  m_path_hash{ld.m_path_hash},
	  m_dir_items{std::move(ld.m_dir_items)},
	  m_cursor{std::move(ld.m_cursor)}
{
	ld.m_path_hash = 0;
	ld.m_cursor_cache = nullptr;
}

List_dir& List_dir::operator=(List_dir&& ld) noexcept
{
	if (this == &ld)
		return *this;

	Column::operator=(std::move(ld));
	m_cursor_cache = ld.m_cursor_cache;
	m_path_hash = ld.m_path_hash;
	m_dir_items = std::move(ld.m_dir_items);
	m_cursor = std::move(ld.m_cursor);
	m_implicit_cursor = ld.m_implicit_cursor;

	ld.m_cursor_cache = nullptr;

	return *this;
}

void List_dir::read_directory()
{
	m_dir_items.clear();

	// Free up some space if we need to.
	constexpr int cache_max_size = 1024;
	if (m_dir_items.size() > cache_max_size)
		m_dir_items.resize(cache_max_size);

	gather_dir_contents(m_path, m_dir_items);
}

bool List_dir::acquire_cursor()
{
	if (m_next_column)
	{
		set_cursor(get_next_path()->filename());
		return false;
	}

	Cursor_cache::Cursor cursor_hash_it;

	if (m_cursor_cache->find(m_path_hash, cursor_hash_it))
	{
		// Ok, so we DID find the hash of the cursor,
		// now let's assign it to the correct Dir_vector
		// item if we can.
		m_cursor = match_cursor_absolute(m_dir_items, cursor_hash_it->second);

		if (m_cursor != m_dir_items.end())
			return false;
	}

	// We didn't find the cursor, let's use the first
	// item of our Dir_vector as the cursor (or the
	// end() iterator if the vector is empty).
	m_cursor = m_dir_items.begin();

	return true;
}

void List_dir::set_cursor(List_dir::Dir_cursor cursor)
{
	m_cursor = std::move(cursor);

	if (m_dir_items.empty())
		return;

	m_cursor_cache->store(m_path_hash, hash_value(m_cursor->path));
	m_implicit_cursor = false;
}

void List_dir::set_cursor(const path& filename)
{
	Dir_cursor cursor = match_cursor_filename(m_dir_items, filename);

	if (cursor != m_dir_items.end())
		set_cursor(cursor);
	else
		set_cursor(m_dir_items.begin());
}

bool List_dir::try_get_cursor(const boost::filesystem::path& filename,
							  List_dir::Dir_cursor& cur)
{
	cur = match_cursor_filename(m_dir_items, filename);

	if (cur != m_dir_items.end())
		return true;
	else
		return false;
}

bool List_dir::try_get_const_cursor(const boost::filesystem::path& filename,
									List_dir::Dir_const_cursor& cur)
{
	List_dir::Dir_cursor c;
	bool ret = try_get_cursor(filename, c);
	cur = c;

	return ret;
}

void List_dir::set_path(const path& dir)
{
	if (dir.empty())
	{
		m_path.clear();
		m_dir_items.clear();
		m_path_hash = 0;

		return;
	}

	using namespace boost::system::errc;

	if (!is_directory(dir))
		throw filesystem_error {dir.native(), make_error_code(not_a_directory)};

	if (access(dir.c_str(), R_OK) == -1)
		throw filesystem_error {dir.native(), make_error_code(permission_denied)};

	Column::set_path(dir);
	m_path_hash = hash_value(dir);

	read_directory();
	m_implicit_cursor = acquire_cursor();
}

} // namespace hawk
