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
#include <exception>
#include "Column.h"
#include "Tab.h"

using namespace hawk;
using namespace boost::filesystem;

Column::Column(const path& p,
	const Type_factory::Type_product& tp)
	:
	m_path{p},
	m_handler_closure{tp},
	m_child_column{}
{}

Column::Column(path&& p,
	const Type_factory::Type_product& tp)
	:
	m_path{std::move(p)},
	m_handler_closure{tp},
	m_child_column{}
{}

Column& Column::operator=(const Column& col)
{
	m_path = col.m_path;
	m_handler = col.m_handler;
	m_handler_closure = col.m_handler_closure;
	m_child_column = col.m_child_column;

	return *this;
}

Column& Column::operator=(Column&& col)
{
	m_path = std::move(col.m_path);
	m_handler = std::move(col.m_handler);
	m_handler_closure = std::move(col.m_handler_closure);
	m_child_column = col.m_child_column;

	return *this;
}

void Column::_ready()
{
	if (m_handler)
	{
		throw std::logic_error
			{ "Handler already created" };
	}

	m_handler =
		std::shared_ptr<Handler>{m_handler_closure(m_path, this)};
}

void Column::_set_child_column(const Column* child_column)
{
	m_child_column = child_column;
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

const path* Column::get_child_path() const
{
	if (!m_child_column)
		return nullptr;
	else
		return &m_child_column->get_path();
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
