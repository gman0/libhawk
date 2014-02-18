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

#ifndef HAWK_LIST_DIR_H
#define HAWK_LIST_DIR_H

#include <vector>
#include <string>
#include <ctime>
#include <utility>
#include <boost/filesystem.hpp>
#include <unordered_map>
#include "Handler.h"
#include "NoHash.h"

namespace hawk {
	class Column;

	class List_dir : public Handler
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

		// The first size_t is the hash of the path to which
		// the cursor belongs to (key) and the second size_t
		// is the item in the directory to which the cursor
		// points to.
		using Cursor_map =
			std::unordered_map<size_t, size_t, No_hash>;

	private:
		// This will hold the hash value of current path
		// which will be used as a key in Tab's cursor map.
		size_t m_path_hash;

		// This flag stores the type of cursor aquisition:
		//  * implicit (true) - aquired as begin/end cursor
		//  * explicit - everything else
		//
		// It can be used e.g. when the resulting item container
		// is sorted (the original vector is NOT sorted) - begin()
		// will point to a different item, thus we need a way to
		// inform the user about this.
		bool m_implicit_cursor;

		Dir_vector m_dir_items;
		Dir_cursor m_cursor;

	public:
		List_dir(const boost::filesystem::path& path,
			Column* parent_column);
		List_dir(const List_dir&) = delete;

		List_dir(List_dir&& ld) noexcept
			:
			Handler{std::move(ld)},
			m_path_hash{ld.m_path_hash},
			m_dir_items{std::move(ld.m_dir_items)},
			m_cursor{std::move(ld.m_cursor)}
		{ ld.m_path_hash = 0; }

		List_dir& operator=(List_dir&& ld);

		// Get a reference to the vector of current directory's contents.
		Dir_vector& get_contents() { return m_dir_items; }
		const Dir_vector& get_contents() const
			{ return m_dir_items; }

		inline bool empty() const { return m_dir_items.empty(); }

		Dir_cursor get_cursor() const { return m_cursor; }
		Dir_vector::const_iterator get_const_cursor() const
			{ return m_cursor; }

		void set_cursor(Dir_cursor cursor);
		void set_cursor(const boost::filesystem::path& cursor);

		virtual void set_path(const boost::filesystem::path& path);
		void set_path(const boost::filesystem::path& path,
			boost::system::error_code& ec) noexcept;

		// (see m_implicit_cursor member)
		inline bool implicit_cursor() const { return m_implicit_cursor; }

	private:
		// returns true if the cursor was aquired implicity,
		// otherwise false
		bool read_directory();
	};
}

#endif // HAWK_LIST_DIR_H
