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

#ifndef HAWK_DIR_CACHE_H
#define HAWK_DIR_CACHE_H

#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include "Path.h"
#include "User_data.h"

namespace hawk
{
	struct Dir_entry
	{
		Path path;
		User_data user_data;
	};

	// Note: existing Dir_ptr's and their Dir_cursor's (Dir_vector
	// iterators) may become invalid after calling On_fs_change.
	// It is advised not to keep Dir_ptr's without calling get_dir_ptr
	// afterwards.
	using Dir_vector = std::vector<Dir_entry>;
	using Dir_cursor = Dir_vector::iterator;
	using Dir_const_cursor = Dir_vector::const_iterator;
	using Dir_ptr = std::shared_ptr<Dir_vector>;

	using On_fs_change = std::function<
		void(const std::vector<size_t>&)>;
	using On_sort_change = std::function<void()>;
	using Dir_sort_predicate = std::function<
		bool(const Dir_entry&, const Dir_entry&)>;
	using Populate_user_data = std::function<void(const Path&, User_data&)>;

	// Starts a thread that checks filesystem every second and
	// updates cache entries if needed.
	void start_filesystem_watchdog(std::chrono::milliseconds update_interval,
								   On_fs_change&& on_fs_change);

	// More or less for internal purposes...
	// ent has to already contain filename of file/directory
	// Populate_user_data functor is then supplied with {base/ent.path}
	void populate_user_data(const Path& base, Dir_entry& ent);

	void set_on_sort_change(On_sort_change&& on_sort_change);
	void set_populate_user_data(Populate_user_data&& populate_ud);
	void set_sort_predicate(Dir_sort_predicate&& pred);

	Dir_sort_predicate get_sort_predicate();

	// Returns a shared pointer with sorted directory entries.
	// The directory path is checked every second by filesystem-watchdog
	// for changes and updated if needed. Access to the particular cache entry
	// is thread-unsafe only during the call to On_fs_change.
	//
	// When the use-count of the shared pointer drops to 1 (i.e. owned only by
	// the filesystem-watchdog), it's marked as free and the memory allocated
	// for the vector pointed to by Dir_ptr may be reused by another cache entry.
	void load_dir_ptr(Dir_ptr& ptr, const Path& directory);

	// Destroy nfree_ptrs free pointers.
	void destroy_free_dir_ptrs(int nfree_ptrs);
}

#endif // HAWK_DIR_CACHE_H
