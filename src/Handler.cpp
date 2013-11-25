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

Handler::Handler(const path& p, const string& type)
	:
	m_path{&p}
{
	m_type = hash<string>()(type);
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
