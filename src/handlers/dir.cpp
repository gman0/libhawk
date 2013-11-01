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
	if (!is_directory(path))
		throw std::runtime_error(
				"Current path is not a directory");

	m_active_cache =
		m_cache.switch_cache(last_write_time(path), hash_value(path));
}

void List_dir::fill_cache(List_dir::Dir_cache* dc)
{
	Dir_vector& vec = dc->vec;
	vec.clear();

	// free up some space if we need to
	if (vec.size() > cache_threshold)
		vec.resize(cache_threshold);

	directory_iterator end;
	for (directory_iterator dir_it{m_path}; dir_it != end; ++dir_it)
	{
		// const path& p = dir_it->path();
		// vec.push_back({last_write_time(p), p, dir_it->status()});
		vec.push_back(dir_it->path());
	}

	dc->cursor = vec.begin();
}
