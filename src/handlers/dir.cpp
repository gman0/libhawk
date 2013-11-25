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
#include <utility>
#include <algorithm>

#include <boost/functional/hash.hpp>
// ^ this resolves an undefined reference to boost::hash_range<>

#include "handlers/dir.h"
#include "handlers/Cache_impl.h"
#include "handlers/dir_hash.h"

using namespace hawk;
using namespace boost::filesystem;

constexpr int cache_threshold = 1024;

List_dir::List_dir(const boost::filesystem::path& path)
	:
	Handler{path, get_handler_hash<List_dir>()},
	m_cache{([this](List_dir::Dir_cache* dc){ fill_cache(dc); })}
{
	if (path.empty())
	{
		m_active_cache = nullptr;
		return;
	}

	if (!is_directory(path))
		throw std::runtime_error
				{ std::string{"Path '"} + path.c_str() + "' is not a directory" };

	m_active_cache =
		m_cache.switch_cache(last_write_time(path), hash_value(path));
}

List_dir& List_dir::operator=(List_dir&& ld)
{
	m_path = ld.m_path;
	m_type = ld.m_type;
	m_cache = std::move(ld.m_cache);
	m_active_cache = ld.m_active_cache;
	ld.m_active_cache = nullptr;

	return *this;
}

void List_dir::fill_cache(List_dir::Dir_cache* dc)
{
	Dir_vector& vec = dc->vec;
	vec.clear();

	// free up some space if we need to
	if (vec.size() > cache_threshold)
		vec.resize(cache_threshold);

	std::copy(directory_iterator {*m_path}, directory_iterator {},
				std::back_inserter(vec));

	dc->cursor = vec.begin();
}

void List_dir::set_cursor(const List_dir::Dir_cursor& cursor)
{
	m_active_cache->cursor = cursor;
}

const List_dir::Dir_cursor& List_dir::get_cursor() const
{
	return m_active_cache->cursor;
}

void List_dir::set_path(const path& dir)
{
	Handler::set_path(dir);

	if (dir.empty())
	{
		m_active_cache = nullptr;
		return;
	}

	if (!is_directory(dir))
	{
		throw std::runtime_error
			{ std::string {"\""} + dir.c_str() + "\" is not a directory" };
	}

	m_path = &dir;
	m_active_cache =
		m_cache.switch_cache(last_write_time(dir), hash_value(dir));
}
