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
#include <iterator>
#include <unistd.h>
#include "handlers/List_dir.h"
#include "handlers/List_dir_hash.h"
#include "View_group.h"
#include "Cursor_cache.h"
#include "View.h"
#include "Filesystem.h"

namespace hawk {

namespace {

Dir_cursor match_cursor(Dir_vector& vec, size_t hash, Dir_cursor cur,
						List_dir::Cursor_search_direction dir)
{
	using Dir = List_dir::Cursor_search_direction;
	auto pred = [hash](const Dir_entry& ent){
		return ent.path.hash() == hash;
	};

	switch (dir)
	{
	case Dir::begin:
		return std::find_if(vec.begin(), vec.end(), pred);
	case Dir::before:
		return (std::find_if(std::reverse_iterator<Dir_cursor>{cur},
							 vec.rend(), pred) + 1).base();
	case Dir::after:
		return std::find_if(cur, vec.end(), pred);
	default: return vec.end();
	}
}

List_dir::Cursor_search_direction determine_direction(
		Dir_cursor cur, const Path& filename, const Path& base)
{
	Dir_entry ent;
	ent.path = filename;
	populate_user_data(base, ent);

	return (get_sort_predicate()(ent, *cur)) ?
				List_dir::Cursor_search_direction::before
			  : List_dir::Cursor_search_direction::after;
}

Dir_cursor match_directed_cursor(
		const Path& filename, Dir_vector& vec,
		Dir_cursor current_cur, List_dir::Cursor_search_direction dir,
		const Path& base)
{
	if (dir == List_dir::Cursor_search_direction::detect)
		dir = determine_direction(current_cur, filename, base);

	return match_cursor(vec, filename.hash(), current_cur, dir);
}

} // unnamed-namespace

void List_dir::acquire_cursor()
{
	if (size_t cursor_hash = m_cursor_cache.find(m_path.hash()))
	{
		// We found the hash of the cursor, now let's assign
		// it to the correct Dir_vector item if we can.
		m_cursor = match_cursor(*m_dir_ptr, cursor_hash, Dir_cursor{},
								Cursor_search_direction::begin);

		if (m_cursor != m_dir_ptr->end())
			return;
	}

	// We didn't find the cursor, let's use the first
	// item of our Dir_vector as the cursor (or the
	// end() iterator if the vector is empty).
	m_cursor = m_dir_ptr->begin();
}

void List_dir::update_cursor_cache()
{
	m_cursor_cache.store(m_path.hash(), m_cursor->path.hash());
}

void List_dir::set_cursor(Dir_cursor cursor)
{
	m_cursor = cursor;

	if (m_dir_ptr->empty())
		return;

	m_cursor_cache.store(m_path.hash(), m_cursor->path.hash());
}

void List_dir::set_cursor(const Path& filename, Cursor_search_direction dir)
{
	Dir_cursor cursor = match_directed_cursor(
				filename, *m_dir_ptr, m_cursor, dir, m_path);

	if (cursor != m_dir_ptr->end())
		set_cursor(cursor);
	else
		set_cursor(m_dir_ptr->begin());
}

void List_dir::advance_cursor(Dir_vector::difference_type d)
{
	if (!m_dir_ptr || m_dir_ptr->empty()) return;

	if (d > 0)
	{
		auto dist = std::distance(m_cursor, m_dir_ptr->end()) - 1;
		if (dist < d)
			d = dist;
	}
	else
	{
		auto dist = std::distance(m_dir_ptr->begin(), m_cursor);
		if (dist < -d)
			d = -dist;
	}

	std::advance(m_cursor, d);
	update_cursor_cache();
}

void List_dir::rewind_cursor(List_dir::Cursor_position pos)
{
	if (!m_dir_ptr || m_dir_ptr->empty()) return;

	if (pos == Cursor_position::beg)
		m_cursor = m_dir_ptr->begin();
	else
		m_cursor = --m_dir_ptr->end();

	update_cursor_cache();
}

bool List_dir::try_get_cursor(const Path& filename, Dir_cursor& cur,
							  Cursor_search_direction dir)
{
	cur = match_directed_cursor(filename, *m_dir_ptr, m_cursor, dir, m_path);
	return cur != m_dir_ptr->end();
}

bool List_dir::try_get_const_cursor(const Path& filename, Dir_const_cursor& cur,
									Cursor_search_direction dir)
{
	Dir_cursor c;
	bool ret = try_get_cursor(filename, c, dir);
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

	Stat st = status(dir);

	if (!is_directory(st))
		throw Filesystem_error {dir, ENOTDIR};

	if (!is_readable(st))
		throw Filesystem_error {dir, EPERM};

	View::set_path(dir);
	load_dir_ptr(m_dir_ptr, dir);
	acquire_cursor();
}

const Dir_vector* List_dir::get_contents() const
{
	return m_dir_ptr.get();
}

} // namespace hawk
