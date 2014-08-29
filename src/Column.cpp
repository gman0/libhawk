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
#include "Column.h"
#include "Tab.h"

using namespace boost::filesystem;

namespace hawk {

void Column::_set_next_column(const Column* next_column)
{
	m_next_column = next_column;
}

const path& Column::get_path() const
{
	return m_path;
}

const path* Column::get_next_path() const
{
	if (!m_next_column)
		return nullptr;
	else
		return &m_next_column->get_path();
}

void Column::set_path(const path& p)
{
	m_path = p;
}

} // namespace hawk
