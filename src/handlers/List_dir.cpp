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

#include <exception>
#include <utility>
#include <algorithm>
#include <unistd.h>
#include "handlers/List_dir.h"
#include "handlers/List_dir_hash.h"
#include "Tab.h"
#include "Cursor_cache.h"
#include "Column.h"
#include "Filesystem.h"

namespace hawk {

static Dir_cursor match_cursor(Dir_vector& vec, size_t hash)
{
	return std::find_if(vec.begin(), vec.end(), [hash](const Dir_entry& ent){
		return ent.path.hash() == hash;
	});
}

void List_dir::acquire_cursor()
{
	if (m_next_column)
	{
		set_cursor(get_next_path()->filename());
		return;
	}

	Cursor_cache::Cursor cursor;
	if (m_cursor_cache->find(m_path.hash(), cursor))
	{
		// Ok, so we DID find the hash of the cursor,
		// now let's assign it to the correct Dir_vector
		// item if we can.
		m_cursor = match_cursor(*m_dir_ptr, cursor->second);

		if (m_cursor != m_dir_ptr->end())
			return;
	}

	// We didn't find the cursor, let's use the first
	// item of our Dir_vector as the cursor (or the
	// end() iterator if the vector is empty).
	m_cursor = m_dir_ptr->begin();
}

void List_dir::set_cursor(Dir_cursor cursor)
{
	m_cursor = std::move(cursor);

	if (m_dir_ptr->empty())
		return;

	m_cursor_cache->store(m_path.hash(), m_cursor->path.hash());
}

void List_dir::set_cursor(const Path& filename)
{
	Dir_cursor cursor = match_cursor(*m_dir_ptr, filename.hash());

	if (cursor != m_dir_ptr->end())
		set_cursor(cursor);
	else
		set_cursor(m_dir_ptr->begin());
}

bool List_dir::try_get_cursor(const Path& filename, Dir_cursor& cur)
{
	cur = match_cursor(*m_dir_ptr, filename.hash());
	return cur != m_dir_ptr->end();
}

bool List_dir::try_get_const_cursor(const Path& filename, Dir_const_cursor& cur)
{
	Dir_cursor c;
	bool ret = try_get_cursor(filename, c);
	cur = c;

	return ret;
}

void List_dir::set_path(const Path& dir)
{
	if (dir.empty())
	{
		m_path.clear();
		m_dir_ptr.reset();

		return;
	}

	struct stat st = status(dir);

	if (!is_directory(st))
		throw Filesystem_error {dir, ENOTDIR};

	if (!is_readable(st))
		throw Filesystem_error {dir, EPERM};

	Column::set_path(dir);
	m_dir_ptr = get_dir_ptr(dir);
	acquire_cursor();
}

} // namespace hawk
