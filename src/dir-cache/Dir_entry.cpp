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
#include <atomic>
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

constexpr int sort_granularity = 1000;

void parallel_sort(
		std::atomic<int>& nt, size_t beg, size_t end, Dir_vector& vec)
{
	auto vbeg_it = vec.begin();

	if (end - beg < sort_granularity)
	{
		std::sort(vbeg_it + beg, vec.end(), callbacks.sort_pred);
		return;
	}

	auto beg_it = vbeg_it + beg;
	auto end_it = vbeg_it + end;

	auto nbeg_it = end_it;
	auto nend_it = nbeg_it + sort_granularity;
	if (nend_it > vbeg_it + vec.size())
		nend_it = vec.end();

	soft_interruption_point();

	std::launch launch_policy = (nt-- > 0)
			? std::launch::async : std::launch::deferred;

	auto f = std::async(launch_policy, &parallel_sort, std::ref(nt),
						nbeg_it - vbeg_it, nend_it - vbeg_it, std::ref(vec));
	std::sort(beg_it, end_it, callbacks.sort_pred);

	nt++;
	f.wait();

	std::inplace_merge(beg_it, nbeg_it, nend_it, callbacks.sort_pred);
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
	{
		std::atomic<int> nt(std::thread::hardware_concurrency());
		parallel_sort(nt, 0, vec.size(), vec);
	}
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

		vec.push_back(std::move(ent));
	}

	if (callbacks.populate_udata)
	{
		for (Dir_entry& ent : vec)
		{
			soft_interruption_point();
			callbacks.populate_udata({dir / ent.path}, ent.user_data);
		}
	}

	hawk::sort(vec);
}

} // namespace hawk
