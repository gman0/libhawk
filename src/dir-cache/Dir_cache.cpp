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

#include <utility>
#include <functional>
#include <memory>
#include "Filesystem.h"
#include "dir-cache/Dir_cache.h"
#include "dir-cache/Cache_storage.h"
#include "dir-watchdog/Dir_watchdog.h"
#include "dir-watchdog/Poll_monitor.h"


namespace hawk {

namespace {

struct
{
	std::unique_ptr<Dir_watchdog> native;
	std::unique_ptr<Dir_watchdog> poll;
} watchdog;

std::unique_ptr<Cache_storage> storage;

struct
{
	On_fs_change on_fs_change;
	On_sort_change on_sort_change;
} callbacks;

void watch_directory(const Path& p)
{
	if (watchdog.poll)
		watchdog.poll->add_path(p);
}

void watchdog_notify(Monitor::Event ev, const Path& p)
{
	storage->mark_dirty(p);
	callbacks.on_fs_change(p);
}

} // unnamed-namespace

void set_on_sort_change(On_sort_change&& fn)
{
	callbacks.on_sort_change = std::move(fn);
}

void init_dir_cache(uint64_t free_threshold)
{
	storage = std::make_unique<Cache_storage>(free_threshold,
		[](const Path& p) noexcept {
			watchdog.poll->remove_path(p);
		});
}

void start_filesystem_watchdog(std::chrono::milliseconds update_interval,
							   On_fs_change&& fn)
{
	callbacks.on_fs_change = std::move(fn);

	auto poll_mon = std::make_unique<Poll_monitor>(update_interval);
	watchdog.poll = std::make_unique<Dir_watchdog>(
				std::move(poll_mon), &watchdog_notify);
}

void load_dir_ptr(Dir_ptr& ptr, const Path& directory, bool force_reload)
{
	ptr.reset();
	if (force_reload) storage->mark_dirty(directory);

	watch_directory(directory);
	ptr = storage->get(directory);
}

} // namespace hawk
