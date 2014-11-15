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
#include "Dir_cache.h"

namespace hawk {
	class Cursor_cache;

	class Tab_manager
	{
	public:
		using Tab_list = std::list<Tab>;
		using Exception_handler = std::function<
			void(std::exception_ptr) noexcept>;

	private:
		Tab_list m_tabs;

		Type_factory* m_type_factory;
		Type_factory::Handler m_list_dir_closure;
		Exception_handler m_exception_handler;

		// default number of columns to create
		unsigned m_ncols;

	public:
		Tab_manager(Type_factory* tf, unsigned ncols,
					Populate_user_data&& populate_user_data,
					Exception_handler&& eh);
		Tab_manager(const Tab_manager&) = delete;
		Tab_manager& operator=(const Tab_manager&) = delete;
		Tab_manager(Tab_manager&&) = delete;
		Tab_manager& operator=(Tab_manager&&) = delete;

		template <typename Path>
		Tab* add_tab(Path&& path)
		{
			auto prev_it = --m_tabs.end();
			m_tabs.emplace_back(std::forward<Path>(path), m_exception_handler,
								m_ncols, m_type_factory, m_list_dir_closure,
								std::chrono::milliseconds{50});
			auto curr_it = --m_tabs.end();

			return (prev_it == curr_it) ? nullptr : &(*curr_it);
		}

		Tab_list& get_tabs();

	private:
		void on_fs_change(const std::vector<size_t>& paths);
		void on_sort_change();
	};
}

#endif // HAWK_TABMANAGER_H
