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

#include <functional>
#include <chrono>
#include "dir-cache/Dir_entry.h"

namespace hawk
{
	using On_fs_change = std::function<
		void(const Path&)>;
	using On_sort_change = std::function<void()>;

	void set_on_sort_change(On_sort_change&& fn);

	// Initialises the directory cache.
	// See dir-cache/Cache_storage.h
	void init_dir_cache(uint64_t free_threshold);

	enum Enabled_watchdogs { WD_POLL = 1, WD_NATIVE = 2 };
	// Watches directories for changes and calls On_fs_change
	// upon a change. See dir-watchdog/Dir_watchdog.h
	void start_filesystem_watchdog(std::chrono::milliseconds update_interval,
								   int enabled_watchdogs,
								   On_fs_change&& fn);

	// Returns a shared pointer with sorted directory entries.
	// The directory path is checked by filesystem-watchdog for changes and
	// updated if needed. Access to the particular cache entry is thread-unsafe
	// only during the call to On_fs_change.
	//
	// When the use-count of the shared pointer drops to 1 (i.e. owned only by
	// directory cache), it's marked as free and the memory allocated
	// for the vector pointed to by Dir_ptr may be reused by another cache entry.
	void load_dir_ptr(Dir_ptr& ptr, const Path& directory,
					  bool force_reload = false);
}

#endif // HAWK_DIR_CACHE_H
