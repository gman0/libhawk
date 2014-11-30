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

#ifndef HAWK_LIST_DIR_H
#define HAWK_LIST_DIR_H

#include <vector>
#include <string>
#include <ctime>
#include <utility>
#include "Column.h"
#include "No_hash.h"
#include "Dir_cache.h"

namespace hawk {
	class Cursor_cache;

	class List_dir : public Column
	{
	private:
		Cursor_cache* m_cursor_cache;

	protected:
		Dir_ptr m_dir_ptr;
		Dir_cursor m_cursor;

	public:
		List_dir(Cursor_cache* cc)
			: m_cursor_cache{cc}
		{}

		List_dir(const List_dir&) = delete;

		const Dir_vector& get_contents() const { return *m_dir_ptr; }

		Dir_cursor get_cursor() const { return m_cursor; }
		Dir_const_cursor get_const_cursor() const { return m_cursor; }

		// These two methods try to find a cursor with supplied filename.
		// (Note that by filename I mean filename by calling eg. Path's
		// filename() method.)
		// On success, the resulting cursor is stored in cur and
		// true is returned.
		bool try_get_cursor(const Path& filename, Dir_cursor& cur);
		bool try_get_const_cursor(const Path& filename, Dir_const_cursor& cur);

		void set_cursor(Dir_cursor cursor);
		void set_cursor(const Path& filename);

		virtual void set_path(const Path& path);

	private:
		void acquire_cursor();
	};
}

#endif // HAWK_LIST_DIR_H
