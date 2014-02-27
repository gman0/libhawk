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

#include <algorithm>
#include <exception>
#include <string>
#include "Type_factory.h"
#include "Handler.h"
#include "calchash.h"

using namespace boost::filesystem;

constexpr int half_size_t = sizeof(size_t) * 4;

namespace hawk {

Type_factory::Magic_guard::Magic_guard()
{
	magic_cookie = magic_open(MAGIC_MIME_TYPE
							// follow the symlinks
							| MAGIC_SYMLINK
							// don't look for known tokens in ASCII files
							| MAGIC_NO_CHECK_TOKENS);
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
	auto it = std::find_if(m_types.begin(), m_types.end(),
		[&type](const Type_map::value_type& v)
		{
			return find_predicate(v, type);
		}
	);

	if (it == m_types.end())
		return Type_product {nullptr};

	// We've got the mime-type category figured out, now let's check
	// if we need to check for the specific type too.
	if (((size_t)~0 >> half_size_t) & it->first)
	{
		// ok we need to check the whole mime type hash
		if (type != it->first)
			return Type_product {nullptr};
	}

	return it->second;
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
	// check only for the category
	static std::string mime;
	mime = get_mime(p);

	return calculate_mime_hash(mime);
}

} //namespace hawk
