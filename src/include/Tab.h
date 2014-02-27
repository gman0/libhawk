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

#ifndef HAWK_TAB_H
#define HAWK_TAB_H

#include <boost/filesystem/path.hpp>
#include <vector>
#include <utility>
#include <exception>
#include "Column.h"
#include "Type_factory.h"
#include "handlers/List_dir.h"
#include "handlers/List_dir_hash_extern.h"

namespace hawk {
	class Type_factory;

	class Tab
	{
	public:
		using Column_vector = std::vector<Column>;

	private:
		// PWD usually stands for Print Working Directory
		// but there's no printing involved as this only
		// holds the current working directory.
		boost::filesystem::path m_pwd;

		Column_vector m_columns;
		Column* m_active_column;

		Type_factory* m_type_factory;
		Type_factory::Type_product m_list_dir_closure;

		// This one's used to check whether we have
		// a preview column. Also, we can make the assumption
		// that only the last column can be a preview and that
		// there can be only one preview.
		bool m_has_preview;

		// This cursor map stores all cursors. It is shared
		// between all List_dir handlers that exist in a
		// particular Tab.
		List_dir::Cursor_map m_cursor_map;

	public:
		Tab(const boost::filesystem::path& pwd,
			unsigned ncols,
			Type_factory* tf,
			const Type_factory::Type_product& list_dir_closure);
		Tab(boost::filesystem::path&& pwd,
			unsigned ncols,
			Type_factory* tf,
			const Type_factory::Type_product& list_dir_closure);
		Tab(const Tab& t);
		Tab(Tab&& t);

		Tab& operator=(const Tab& t);
		Tab& operator=(Tab&& t);

		const boost::filesystem::path& get_pwd() const;
		void set_pwd(const boost::filesystem::path& pwd);
		void set_pwd(const boost::filesystem::path& pwd,
			boost::system::error_code& ec) noexcept;

		void add_column(const boost::filesystem::path& pwd);
		void remove_column();

		Column_vector& get_columns();
		const Column_vector& get_columns() const;

		Column* get_active_column() { return m_active_column; }
		const Column* get_active_column() const { return m_active_column; }
		size_t get_current_ncols(); // don't confuse this with the size of column vector

		List_dir::Dir_cursor get_begin_cursor() const;
		void set_cursor(List_dir::Dir_cursor cursor);
		void set_cursor(List_dir::Dir_cursor cursor,
			boost::system::error_code& ec) noexcept;

		// Tries to find a cursor with key cursor_hash.
		// Returns true on success (that is result != m_cursor_map.end()).
		// The resulting find iterator is stored in iter.
		bool find_cursor(size_t cursor_hash,
			List_dir::Cursor_map::iterator& iter);

		// Stores a cursor in the cursor map.
		void store_cursor(size_t key, size_t cursor_hash);

		List_dir::Cursor_map&
			get_cursor_map() { return m_cursor_map; }

	private:
		void build_columns(unsigned ncols);
		void update_paths(boost::filesystem::path pwd);

		void add_column(const boost::filesystem::path& pwd,
			const Type_factory::Type_product& closure);
		void add_column(boost::filesystem::path&& pwd,
			const Type_factory::Type_product& closure);
		void add_column(const boost::filesystem::path& pwd,
			const Type_factory::Type_product& closure,
			unsigned inplace_col);

		inline void activate_last_column()
		{
			m_active_column = &(m_columns.back());
		}

		List_dir* get_list_dir_handler(Handler* handler);
		inline const boost::filesystem::path* get_last_column_path() const;

		void update_active_cursor();

		// Updates pointers to this instance of Tab of all columns
		// by calling their _set_parent_tab methods.
		void update_cols_tab_ptr();
	};
}

#endif // HAWK_TAB_H
