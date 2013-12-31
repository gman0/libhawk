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

#include <functional>
#include "Handler.h"

using namespace std;
using namespace hawk;
using namespace boost::filesystem;

Handler::Handler(const path& p, Column* parent_column,
	const string& type)
	:
	m_path{&p},
	m_parent_column{parent_column}
{
	m_type = hash<string>()(type);
}

Handler::Handler(Column* parent_column,
	const string& type)
	:
	m_path{},
	m_parent_column{parent_column}
{
	m_type = hash<string>()(type);
}

Handler& Handler::operator=(Handler&& h)
{
	m_path = h.m_path;
	m_type = h.m_type;
	m_parent_column = h.m_parent_column;

	h.m_path = nullptr;
	h.m_parent_column = nullptr;
	
	return *this;
}

size_t Handler::get_type() const
{
	return m_type;
}

bool Handler::operator==(const Handler& h)
{
	return (m_type == h.m_type);
}

bool Handler::operator!=(const Handler& h)
{
	return (m_type != h.m_type);
}

void Handler::set_path(const path& p)
{
	m_path = &p;
}

const Column* Handler::get_parent_column() const
{
	return m_parent_column;
}
