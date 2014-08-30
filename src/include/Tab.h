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

#include <vector>
#include <memory>
#include <boost/filesystem/path.hpp>
#include "Type_factory.h"
#include "handlers/List_dir.h"

namespace hawk {
	class Cursor_cache;
	class Column;

	class Tab
	{
	public:
		using Column_vector = std::vector<std::unique_ptr<Column>>;

	private:
		boost::filesystem::path m_path;

		Column_vector m_columns;
		List_dir* m_active_ld;

		Type_factory* m_type_factory;
		Type_factory::Handler m_list_dir_closure;

		// This one's used to check whether we have
		// a preview column. Also, we can make the assumption
		// that only the last column can be a preview and that
		// there can be only one preview.
		bool m_has_preview;

		// Cursor_cache is shared between all List_dir handlers
		// that exist in a particular Tab.
		Cursor_cache* m_cursor_cache;

	public:
		Tab(const boost::filesystem::path& path,
			Cursor_cache* cc,
			int ncols,
			Type_factory* tf,
			const Type_factory::Handler& list_dir_closure);
		Tab(boost::filesystem::path&& path,
			Cursor_cache* cc,
			int ncols,
			Type_factory* tf,
			const Type_factory::Handler& list_dir_closure);

		const boost::filesystem::path& get_path() const;
		void set_path(boost::filesystem::path path);

		// Removes the leftmost (first) column.
		void remove_column();

		const Column_vector& get_columns() const;
		List_dir* const get_active_list_dir() const;

		void set_cursor(List_dir::Dir_cursor cursor);

	private:
		void build_columns(int ncols);
		void instantiate_columns(int ncols);
		void init_column_paths(int ncols);

		void update_paths(boost::filesystem::path path);

		void add_column(const Type_factory::Handler& closure);
		void add_column(const boost::filesystem::path& p,
						const Type_factory::Handler& closure);

		void update_active_cursor();
		void activate_last_column();
	};
}

#endif // HAWK_TAB_H
