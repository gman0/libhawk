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
#include "handlers/List_dir_hash_extern.h"

namespace hawk {

Tab_manager::Tab_manager(Type_factory* tf, unsigned ncols)
	:
	m_type_factory{tf},
	m_ncols{ncols}
{
	m_list_dir_closure = (*tf)[get_handler_hash<List_dir>()];

	if (!m_list_dir_closure)
		throw std::logic_error { "No List_dir handler registered" };
}

Tab_manager::Tab_iterator& Tab_manager::get_active_tab()
{
	return m_active_tab;
}

void Tab_manager::set_active_tab(Tab_manager::Tab_iterator& tab)
{
	m_active_tab = tab;
}

void Tab_manager::set_active_tab(Tab_manager::Tab_iterator&& tab)
{
	m_active_tab = std::move(tab);
}

Tab_manager::Tab_iterator& Tab_manager::add_tab()
{
	m_tabs.emplace_back(m_active_tab->get_path(), m_ncols,
						m_type_factory, m_list_dir_closure);
	return (m_active_tab = --m_tabs.end());
}

Tab_manager::Tab_iterator& Tab_manager::add_tab(
		const boost::filesystem::path& pwd)
{
	m_tabs.emplace_back(pwd, m_ncols, m_type_factory, m_list_dir_closure);
	return (m_active_tab = --m_tabs.end());
}

Tab_manager::Tab_iterator& Tab_manager::add_tab(boost::filesystem::path&& pwd)
{
	m_tabs.emplace_back(
			std::move(pwd), m_ncols, m_type_factory, m_list_dir_closure);
	return (m_active_tab = --m_tabs.end());
}

void Tab_manager::remove_tab(Tab_manager::Tab_iterator& tab)
{
	if (m_tabs.size() <= 1) return; // don't delete our only tab!

	// if we're deleting the first tab, make the next one active,
	// otherwise the previouse one will be active
	m_active_tab = (tab == m_tabs.begin()) ? tab++ : tab--;
	m_tabs.erase(tab);
}

int Tab_manager::count() const
{
	return m_tabs.size();
}

} // namespace hawk
