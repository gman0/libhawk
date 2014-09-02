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
#include <boost/filesystem.hpp>
#include "Column.h"
#include "No_hash.h"

namespace hawk {
	class Cursor_cache;

	class List_dir : public Column
	{
	public:
		struct Dir_entry
		{
			std::time_t timestamp;
			boost::filesystem::path path;
			boost::filesystem::file_status status;
		};

		using Dir_vector = std::vector<Dir_entry>;
		using Dir_cursor = Dir_vector::iterator;
		using Dir_const_cursor = Dir_vector::const_iterator;

	private:
		Cursor_cache* m_cursor_cache;

		// This will hold the hash value of current path
		// which will be used as a key in m_cursor_cache.
		size_t m_path_hash;

		// This flag stores the type of cursor acquisition:
		//  * implicit (true) - acquired as begin/end cursor
		//  * explicit - everything else
		//
		// It can be used e.g. when the resulting item container
		// is sorted (the original vector is NOT sorted) - begin()
		// will point to a different item, thus we need a way to
		// inform the user about this.
		bool m_implicit_cursor;
	protected:
		Dir_vector m_dir_items;
		Dir_cursor m_cursor;

	public:
		List_dir(Cursor_cache* cc)
			:
			  m_cursor_cache{cc},
			  m_path_hash{0},
			  m_implicit_cursor{true}
		{}

		List_dir(const List_dir&) = delete;
		List_dir(List_dir&& ld) noexcept;
		List_dir& operator=(List_dir&& ld) noexcept;

		// Get a reference to the vector of current directory's contents.
		Dir_vector& get_contents() { return m_dir_items; }
		const Dir_vector& get_contents() const { return m_dir_items; }

		Dir_cursor get_cursor() const { return m_cursor; }
		Dir_const_cursor get_const_cursor() const { return m_cursor; }

		// These two methods try to find a cursor with supplied filename.
		// (Note that by filename I mean filename by calling eg.
		// boost::filesystem::path's filename() method.)
		// On success, the resulting cursor is stored in cur and
		// true is returned.
		bool try_get_cursor(const boost::filesystem::path& filename,
							Dir_cursor& cur);
		bool try_get_const_cursor(const boost::filesystem::path& filename,
								  Dir_const_cursor& cur);

		void set_cursor(Dir_cursor cursor);
		void set_cursor(const boost::filesystem::path& filename);

		virtual void set_path(const boost::filesystem::path& path);

		// (see m_implicit_cursor member)
		bool implicit_cursor() const { return m_implicit_cursor; }

	private:
		void read_directory();

		// Returns true if the cursor was acquired implicity,
		// otherwise false.
		bool acquire_cursor();
	};
}

#endif // HAWK_LIST_DIR_H
