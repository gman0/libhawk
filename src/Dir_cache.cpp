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

#include <utility>
#include <list>
#include <boost/filesystem.hpp>
#include "Dir_cache.h"

using namespace boost::filesystem;

namespace hawk {

struct Cache_entry
{
	size_t	hash;
	Dir_ptr	ptr;
};

using List = std::list<Cache_entry>;
using List_iter = List::iterator;
static List s_entries;
static int s_nfree = 0;

static bool find_entry(List& l, List_iter& it, size_t path_hash)
{
	it = std::find_if(l.begin(), l.end(),
					  [path_hash](const Cache_entry& entry)
					  { return entry.hash == path_hash; });

	return it != l.end();
}

static bool is_free(List_iter& it)
{
	return it->hash == 0;
}

// Don't waste space.
static void shrink(Dir_ptr& ptr)
{
	constexpr unsigned max_size = 1024;
	ptr->resize(max_size);
}

static void cmp_and_set(List_iter& out, List_iter& other)
{
	if (s_nfree == 1)
		out = other;
	else
	{
		// Try to get a larger vector.
		if (other->ptr->size() > out->ptr->size())
			out = other;
	}
}

static bool prepare_and_find_free_ptrs(List& l, List_iter& out)
{
	s_nfree = 0;

	for (auto it = l.begin(); it != l.end(); ++it)
	{
		if (it->ptr.use_count() == 1)
		{
			++s_nfree;
			it->hash = 0;

			shrink(it->ptr);
			cmp_and_set(out, it);
		}
	}

	return s_nfree;
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

Dir_ptr get_dir_ptr(const path& p, size_t path_hash)
{
	List_iter find;
	if (find_entry(s_entries, find, path_hash))
		return find->ptr;
	else
	{
		List_iter find_free;
		if (prepare_and_find_free_ptrs(s_entries, find_free))
		{
			find_free->hash = path_hash;
			find_free->ptr->clear();
			gather_dir_contents(p, *find_free->ptr);

			return find_free->ptr;
		}
	}

	Cache_entry ent {path_hash, std::make_shared<Dir_vector>()};
	gather_dir_contents(p, *ent.ptr);
	s_entries.push_back(ent);

	return ent.ptr;
}

void destroy_free_dir_ptrs(int free_ptrs)
{
	if (--free_ptrs > 0)
	{
		List_iter it = s_entries.begin();
		List_iter next;
		while (it != s_entries.end() && free_ptrs >= 0)
		{
			next = it;
			++next;

			if (is_free(it))
			{
				s_entries.erase(it);
				--free_ptrs;
			}

			it = next;
		}
	}
}

} // namespace hawk
