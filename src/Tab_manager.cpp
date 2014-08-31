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
#include "Tab_manager.h"
#include "handlers/List_dir_hash.h"

using boost::filesystem::path;

namespace hawk {

using Tab_iterator = Tab_manager::Tab_list::iterator;

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

} // namespace hawk
