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
#include "Tab.h"

namespace hawk {
	class Tab_manager
	{
	public:
		using Tab_iterator = typename std::list<Tab>::iterator;

	private:
		std::list<Tab> m_tabs;
		Tab_iterator m_active_tab;

		Type_factory* m_type_factory;
		Type_factory::Type_product m_list_dir_closure;

		// default number of columns to create
		unsigned m_ncols;

	public:
		Tab_manager(Type_factory* tf, unsigned ncols);
		Tab_manager(const Tab_manager&) = delete;
		Tab_manager& operator=(const Tab_manager&) = delete;

		Tab_iterator& get_active_tab();
		void set_active_tab(Tab_iterator& tab);
		void set_active_tab(Tab_iterator&& tab);

		Tab_iterator& add_tab();
		Tab_iterator& add_tab(const boost::filesystem::path& pwd);
		Tab_iterator& add_tab(boost::filesystem::path&& pwd);
		void remove_tab(Tab_iterator& tab);
		int  count() const;
	};
}

#endif // HAWK_TABMANAGER_H
