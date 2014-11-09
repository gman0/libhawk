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

#include <utility>
#include <cassert>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include "Tab_manager.h"
#include "handlers/List_dir_hash.h"

using boost::filesystem::path;

namespace hawk {

using Tab_iterator = Tab_manager::Tab_list::iterator;
using Hash_vector = std::vector<size_t>;

Tab_manager::Tab_manager(Type_factory* tf, unsigned ncols,
						 Populate_user_data&& populate_user_data,
						 Exception_handler&& eh)
	:
	  m_type_factory{tf},
	  m_exception_handler{eh},
	  m_ncols{ncols}
{
	m_list_dir_closure = tf->get_handler(hash_list_dir());

	assert(m_list_dir_closure && "No List_dir handler registered");

	auto fs_change = [this](const Hash_vector& hvec) {
		on_fs_change(hvec);
	};
	auto sort_change = [this]{ on_sort_change(); };
	_start_filesystem_watchdog(fs_change, sort_change,
							   std::move(populate_user_data));
}

void Tab_manager::on_fs_change(const Hash_vector& hvec)
{
	std::vector<Tab_list::iterator> tabs;
	for (size_t path_hash : hvec)
	{
		assert(path_hash != 0);

		auto tab_it = m_tabs.begin();
		for (; tab_it != m_tabs.end(); ++tab_it)
		{
			const Tab::Column_vector& col_vec = tab_it->get_columns();

			// Try to find a columns with the supplied path.
			// It's more likely that the result will be found quicker
			// when we'll be searching for it in reverse order.
			const auto col_it = std::find_if(
						col_vec.crbegin(), col_vec.crend(),
						[path_hash](const Tab::Column_ptr& col) {
					return hash_value(col->get_path()) == path_hash;
			});

			if (col_it != col_vec.rend())
				break;
		}

		if (tab_it != m_tabs.end())
			tabs.push_back(tab_it);
	}

	std::unique(tabs.begin(), tabs.end());
	for (auto tab_it : tabs)
		tab_it->set_path(tab_it->get_path());
}

void Tab_manager::on_sort_change()
{
	for (Tab& t : m_tabs)
		t.set_path(t.get_path());
}

} // namespace hawk
