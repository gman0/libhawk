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

#ifndef HAWK_TAB_MANAGER_H
#define HAWK_TAB_MANAGER_H

#include <list>
#include <vector>
#include "Tab.h"

namespace hawk {
	class Cursor_cache;

	class Tab_manager
	{
	public:
		using Tab_list = std::list<Tab>;

	private:
		Tab_list m_tabs;

		Type_factory* m_type_factory;
		Type_factory::Handler m_list_dir_closure;

		// default number of columns to create
		unsigned m_ncols;

	public:
		Tab_manager(Type_factory* tf, unsigned ncols);
		Tab_manager(const Tab_manager&) = delete;
		Tab_manager& operator=(const Tab_manager&) = delete;
		Tab_manager(Tab_manager&&) = delete;
		Tab_manager& operator=(Tab_manager&&) = delete;

		// These can return nullptr if the tab construction fails.
		Tab* add_tab(const boost::filesystem::path& pwd, Cursor_cache* cc);
		Tab* add_tab(boost::filesystem::path&& pwd, Cursor_cache* cc);

		Tab_list& get_tabs();

	private:
		void on_fs_change(const std::vector<size_t>& paths);
	};
}

#endif // HAWK_TABMANAGER_H
