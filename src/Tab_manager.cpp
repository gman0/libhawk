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
#include <exception>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include "Tab_manager.h"
#include "handlers/List_dir_hash.h"
#include "Dir_cache.h"

#include <cassert>

using boost::filesystem::path;

namespace hawk {

using Tab_iterator = Tab_manager::Tab_list::iterator;
using Hash_vector = std::vector<size_t>;

static Tab* get_tab_ptr(Tab_iterator& prev, Tab_iterator& curr)
{
	// Return nullptr if the Tab construction failed.
	if (prev == curr)
		return nullptr;
	else
		return &(*curr);
}

Tab_manager::Tab_manager(Type_factory* tf, unsigned ncols)
	:
	  m_type_factory{tf},
	  m_ncols{ncols}
{
	m_list_dir_closure = tf->get_handler(hash_list_dir());

	if (!m_list_dir_closure)
		throw std::logic_error { "No List_dir handler registered" };

	start_filesystem_watchdog([this](const Hash_vector& hvec) {
		on_fs_change(hvec);
	});
}

Tab* Tab_manager::add_tab(const path& pwd, Cursor_cache* cc)
{
	Tab_iterator it = --m_tabs.end();
	m_tabs.emplace_back(pwd, cc, m_ncols, m_type_factory, m_list_dir_closure);

	return get_tab_ptr(it, --m_tabs.end());
}

Tab* Tab_manager::add_tab(path&& pwd, Cursor_cache* cc)
{
	Tab_iterator it = --m_tabs.end();
	m_tabs.emplace_back(std::move(pwd), cc, m_ncols, m_type_factory,
						m_list_dir_closure);

	return get_tab_ptr(it, --m_tabs.end());
}

void Tab_manager::on_fs_change(const Hash_vector& hvec)
{
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
			const auto col_it = std::find_if(col_vec.crbegin(), col_vec.crend(),
									   [path_hash](const Tab::Column_ptr& col){
				return hash_value(col->get_path()) == path_hash;
			});

			if (col_it != col_vec.rend())
				break;
		}

		if (tab_it != m_tabs.end())
			tab_it->reload_current_path();
	}
}

} // namespace hawk
