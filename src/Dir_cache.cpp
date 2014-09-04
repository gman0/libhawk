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

#include <unordered_map>
#include <utility>
#include <boost/filesystem.hpp>
#include "Dir_cache.h"

using namespace boost::filesystem;

namespace hawk {

using Map = std::unordered_map<size_t,
				std::shared_ptr<Dir_vector>, No_hash>;
using Ptr = std::shared_ptr<Dir_vector>;

static Map s_src;
static unsigned s_ptrs_in_use = 0;

static int get_free_ptrs()
{ return s_src.size() - s_ptrs_in_use; }

static bool is_free(Map::iterator it)
{ return (it->second.use_count() < 3); }

Dir_ptr::Dir_ptr(Ptr& ptr) : m_ptr{ptr}
{
	++s_ptrs_in_use;
}

Dir_ptr::~Dir_ptr()
{
	if (!m_ptr)
		return;

	--s_ptrs_in_use;

	int free_ptrs = get_free_ptrs();
	if (free_ptrs > 0)
	{
		for (auto it = s_src.begin(); it != s_src.end(); ++it)
		{
			if (is_free(it))
			{
				s_src.erase(it);
				--free_ptrs;
			}
		}
	}
}

Dir_ptr& Dir_ptr::operator=(Dir_ptr&& ptr) noexcept
{
	m_ptr = std::move(ptr.m_ptr);
	return *this;
}


// Gather contents of a directory and move them to the out vector
static void gather_dir_contents(const path& p, Dir_vector& out)
{
	directory_iterator dir_it_end;
	for (auto dir_it = directory_iterator {p};
		 dir_it != dir_it_end;
		 ++dir_it)
	{
		Dir_entry dir_ent;
		dir_ent.path = std::move(*dir_it);

		boost::system::error_code ec;
		dir_ent.status = status(dir_ent.path, ec);

		out.push_back(std::move(dir_ent));
	}
}

static Dir_ptr make_dir_ptr(const path& p, size_t path_hash)
{
	Ptr vec_ptr = std::make_shared<Dir_vector>();
	gather_dir_contents(p, *vec_ptr);
	s_src[path_hash] = vec_ptr;

	return Dir_ptr(vec_ptr);
}

Dir_ptr get_dir_ptr(const path& p, size_t path_hash)
{
	auto it = s_src.find(path_hash);

	if (it == s_src.end())
	{
		if (get_free_ptrs() > 0)
		{
			for (auto it = s_src.begin(); it != s_src.end(); ++it)
			{
				if (!is_free(it)) continue;

				Ptr vec_ptr = it->second;
				vec_ptr->clear();

				return Dir_ptr(vec_ptr);
			}
		}
	}
	else
		return Dir_ptr(it->second);

	return make_dir_ptr(p, path_hash);
}

} // namespace hawk
