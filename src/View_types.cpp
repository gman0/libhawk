/*
	Copyright (C) 2013-2015 Róbert "gman" Vašek <gman@codefreax.org>

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
#include <magic.h>
#include "View_types.h"
#include "calchash.h"

constexpr int half_size_t = sizeof(size_t) * 4;

namespace hawk {

using Type_map = std::unordered_map<size_t, View_types::Handler>;

struct Magic_guard
{
	magic_t magic_cookie;

	Magic_guard()
	{
		magic_cookie =
				magic_open(MAGIC_MIME_TYPE
						   | MAGIC_SYMLINK
						   | MAGIC_NO_CHECK_TOKENS);
		magic_load(magic_cookie, nullptr);
	}

	~Magic_guard()
	{
		magic_close(magic_cookie);
	}
};

static bool find_predicate(const Type_map::value_type& v,
		size_t find)
{
	constexpr int half_size_t = sizeof(size_t) * 4;
	return (v.first >> half_size_t) == ((v.first >> half_size_t)
			& (find >> half_size_t));
}

View_types::View_types()
	: m_magic_guard{new Magic_guard}
{
	if (!m_magic_guard->magic_cookie)
	{
		delete m_magic_guard;
		throw std::runtime_error { std::string {"Cannot load magic database: "}
								  + magic_error(m_magic_guard->magic_cookie) };
	}
}

View_types::~View_types()
{
	delete m_magic_guard;
}

void View_types::register_type(size_t type,
	const View_types::Handler& tp)
{
	m_types[type] = tp;
}

View_types::Handler View_types::get_handler(size_t type) const
{
	auto it = std::find_if(m_types.begin(), m_types.end(),
		[type](const Type_map::value_type& v) {
			return find_predicate(v, type);
	});

	if (it == m_types.end())
		return Handler {nullptr};

	// We've got the mime-type category figured out, now let's check
	// if we need to check for the specific type too.
	if (((size_t)~0 >> half_size_t) & it->first)
	{
		// ok we need to check the whole mime type hash
		if (type != it->first)
			return Handler {nullptr};
	}

	return it->second;
}

View_types::Handler View_types::get_handler(const Path& p) const
{
	return get_handler(get_hash_type(p));
}

const char* View_types::get_mime(const Path& p) const
{
	return magic_file(m_magic_guard->magic_cookie, p.c_str());
}

size_t View_types::get_hash_type(const Path& p) const
{
	static std::string mime;
	const char* m = get_mime(p);

	if (m == nullptr)
		return 0;

	mime = m;

	return calculate_mime_hash(mime);
}

} //namespace hawk
