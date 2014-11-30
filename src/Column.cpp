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

namespace hawk {

Column::Column(Column&& col) noexcept
	:
	  m_path{std::move(col.m_path)},
	  m_next_column{col.m_next_column}
{
	col.m_next_column = nullptr;
}

Column& Column::operator=(Column&& col) noexcept
{
	if (&col == this)
		return *this;

	m_path = std::move(col.m_path);
	m_next_column = col.m_next_column;

	col.m_next_column = nullptr;

	return *this;
}

void Column::_set_next_column(const Column* next_column)
{
	m_next_column = next_column;
}

const Path& Column::get_path() const
{
	return m_path;
}

const Path* Column::get_next_path() const
{
	return m_next_column ? &m_next_column->get_path() : nullptr;
}

void Column::set_path(const Path& p)
{
	m_path = p;
}

} // namespace hawk
