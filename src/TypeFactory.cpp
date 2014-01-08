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

#include <exception>
#include <string>
#include "TypeFactory.h"
#include "Handler.h"

using namespace boost::filesystem;

namespace hawk {

Type_factory::Magic_guard::Magic_guard()
{
	magic_cookie = magic_open(MAGIC_MIME_TYPE);
	magic_load(magic_cookie, nullptr);
}

Type_factory::Magic_guard::~Magic_guard()
{
	magic_close(magic_cookie);
}

Type_factory::Type_factory()
{
	if (!m_magic_guard.magic_cookie)
	{
		throw std::runtime_error
			{
				std::string {"Cannot load magic database: "}
				+ magic_error(m_magic_guard.magic_cookie)
			};
	}
}

void Type_factory::register_type(size_t type,
	const Type_factory::Type_product& tp)
{
	m_types[type] = tp;
}

Type_factory::Type_product Type_factory::operator[](size_t type)
{
	auto tp_iterator = m_types.find(type);

	if (tp_iterator == m_types.end())
		return Type_product {nullptr};

	return tp_iterator->second;
}

Type_factory::Type_product Type_factory::operator[](const path& p)
{
	return operator[](get_hash_type(p));
}

const char* Type_factory::get_mime(const path& p)
{
	return magic_file(m_magic_guard.magic_cookie, p.c_str());
}

size_t Type_factory::get_hash_type(const path& p)
{
	return std::hash<std::string>()({get_mime(p)});
}

} //namespace hawk
