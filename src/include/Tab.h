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

		Type_factory* m_type_factory;

	public:
		Tab(const boost::filesystem::path& pwd, unsigned cols,
			Type_factory* tf);
		Tab(boost::filesystem::path&& pwd, unsigned cols);

		Tab(const Tab& t)
			:
			m_pwd{t.m_pwd},
			m_columns{t.m_columns},
			m_type_factory{t.m_type_factory}
		{}

		Tab(Tab&& t)
			:
			m_pwd{std::move(t.m_pwd)},
			m_columns{std::move(t.m_columns)},
			m_type_factory{t.m_type_factory}
		{}

		Tab& operator=(const Tab& t);
		Tab& operator=(Tab&& t);

		const boost::filesystem::path& get_pwd() const;
		void set_pwd(const boost::filesystem::path& pwd);
	};
}

#endif // HAWK_TAB_H
