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
#include <future>
#include "Interrupt.h"
#include "Filesystem.h"
#include "dir-cache/Dir_entry.h"

namespace hawk {

namespace {

struct
{
	Dir_sort_predicate sort_pred;
	Populate_user_data populate_udata;
} callbacks;

constexpr int sort_granularity = 10000;

void parallel_sort(size_t beg, size_t end, Dir_vector& vec)
{
	Dir_vector::iterator start_it = vec.begin();

	size_t d = end - beg;
	if (d < sort_granularity)
		std::sort(start_it + beg, start_it + end, callbacks.sort_pred);
	else
	{
		const size_t sz = vec.size();
		size_t next_beg = end;
		size_t next_end = next_beg + sort_granularity;

		if (next_end > sz)
			next_end = sz;

		auto beg_it = start_it + beg;
		auto next_beg_it = start_it + next_beg;
		auto next_end_it = start_it + next_end;

		soft_interruption_point();

		auto f = std::async(std::launch::async, parallel_sort, next_beg,
							next_end, std::ref(vec));
		std::sort(beg_it, start_it + end, callbacks.sort_pred);
		f.wait();

		std::inplace_merge(beg_it, next_beg_it,
						   next_end_it, callbacks.sort_pred);
	}
}

} // unnamed-namespace

void set_populate_user_data(Populate_user_data&& populate_udata)
{
	callbacks.populate_udata = std::move(populate_udata);
}

void set_sort_predicate(Dir_sort_predicate&& pred)
{
	callbacks.sort_pred = std::move(pred);
}

Dir_sort_predicate get_sort_predicate()
{
	return callbacks.sort_pred;
}

void sort(Dir_vector& vec)
{
	if (callbacks.sort_pred)
		parallel_sort(0, vec.size(), vec);
}

void populate_user_data(const Path& parent, Dir_entry& ent)
{
	if (callbacks.populate_udata)
		callbacks.populate_udata({parent / ent.path}, ent.user_data);
}

void populate_directory(Dir_vector& vec, const Path& dir)
{
	for (auto it = Directory_iterator {dir}; !it.at_end(); ++it)
	{
		soft_interruption_point();

		Dir_entry ent;
		ent.path = *it;
		populate_user_data(dir, ent);

		vec.push_back(std::move(ent));
	}

	sort(vec);
}

} // namespace hawk
