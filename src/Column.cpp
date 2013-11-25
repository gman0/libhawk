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

#include <utility>
#include "Column.h"

using namespace hawk;
using namespace boost::filesystem;

Column::Column(const path& path,
	const Type_factory::Type_product& tp)
	:
	m_path{path},
	m_handler{tp(path)}
{}

Column& Column::operator=(const Column& col)
{
	m_path = col.m_path;
	m_handler = col.m_handler;

	return *this;
}

Column& Column::operator=(Column&& col)
{
	m_path = std::move(col.m_path);
	m_handler = std::move(col.m_handler);

	return *this;
}

Handler* Column::get_handler()
{
	return m_handler.get();
}

const Handler* Column::get_handler() const
{
	return m_handler.get();
}

const path& Column::get_path() const
{
	return m_path;
}

void Column::set_path(const path& p)
{
	m_path = p;
	m_handler->set_path(m_path);
}

void Column::set_path(path&& p)
{
	m_path = std::move(p);
	m_handler->set_path(m_path);
}
