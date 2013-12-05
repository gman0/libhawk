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

#ifndef HAWK_TAB_H
#define HAWK_TAB_H

#include <boost/filesystem/path.hpp>
#include <vector>
#include <utility>
#include "Column.h"
#include "TypeFactory.h"

namespace hawk {
	class Type_factory;

	class Tab
	{
	private:
		// PWD usually stands for Print Working Directory
		// but there's no printing involved as this only
		// holds the current working directory.
		boost::filesystem::path m_pwd;

		std::vector<Column> m_columns;
		Column* m_active_column;

		Type_factory* m_type_factory;
		Type_factory::Type_product m_list_dir_closure;

		// This one's used to check whether we have
		// a preview column. Also, we can make the assumption
		// that only the last column can be a preview and that
		// there can be only one preview.
		bool m_has_preview;

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

		Tab(Tab&& t)
			:
			m_pwd{std::move(t.m_pwd)},
			m_columns{std::move(t.m_columns)},
			m_active_column{t.m_active_column},
			m_type_factory{t.m_type_factory},
			m_has_preview{t.m_has_preview}
		{ t.m_active_column = nullptr; }

		Tab& operator=(const Tab& t);
		Tab& operator=(Tab&& t);

		const boost::filesystem::path& get_pwd() const;
		void set_pwd(const boost::filesystem::path& pwd);

		void add_column(const boost::filesystem::path& pwd);
		void remove_column();

		std::vector<Column>& get_columns();
		const std::vector<Column>& get_columns() const;

		Column* get_active_column() { return m_active_column; }
		const Column* get_active_column() const { return m_active_column; }
		size_t get_active_column_num();

		void set_cursor(const List_dir::Dir_cursor& cursor);

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

		void update_cursor();
	};
}

#endif // HAWK_TAB_H
