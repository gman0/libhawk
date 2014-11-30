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

#ifndef HAWK_DIR_CACHE_H
#define HAWK_DIR_CACHE_H

#include <memory>
#include <functional>
#include <vector>
#include "Path.h"
#include "User_data.h"

namespace hawk
{
	struct Dir_entry
	{
		Path path;
		User_data user_data;
	};

	using Dir_vector = std::vector<Dir_entry>;
	using Dir_cursor = Dir_vector::iterator;
	using Dir_const_cursor = Dir_vector::const_iterator;
	using Dir_ptr = std::shared_ptr<Dir_vector>;


	using On_fs_change_f = std::function<
		void(const std::vector<size_t>&)>;
	using On_sort_change_f = std::function<void()>;
	using Dir_sort_predicate = std::function<
		bool(const Dir_entry&, const Dir_entry&)>;
	using Populate_user_data = std::function<void(const Path&, User_data&)>;

	// Starts a thread that checks filesystem every second and
	// updates cache entries if needed.
	void _start_filesystem_watchdog(On_fs_change_f&& on_fs_change,
									On_sort_change_f&& on_sort_change,
									Populate_user_data&& populate);

	void set_sort_predicate(Dir_sort_predicate&& pred);

	Dir_ptr get_dir_ptr(const Path& p);

	// Destroy nfree_ptrs free pointers.
	void destroy_free_dir_ptrs(int nfree_ptrs);
}

#endif // HAWK_DIR_CACHE_H
