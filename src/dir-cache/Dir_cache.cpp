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
#include <sys/vfs.h>
#include </usr/include/linux/magic.h>
#include "Filesystem.h"
#include "dir-cache/Dir_cache.h"
#include "dir-cache/Cache_storage.h"
#include "dir-watchdog/Dir_watchdog.h"
#include "dir-watchdog/Poll_monitor.h"
#include "dir-watchdog/Inotify_monitor.h"

namespace hawk {

namespace {

struct
{
	std::unique_ptr<Dir_watchdog> native;
	std::unique_ptr<Dir_watchdog> poll;
} watchdog;

struct
{
	On_fs_change on_fs_change;
	On_sort_change on_sort_change;
} callbacks;

std::unique_ptr<Cache_storage> storage;

void watch_directory(const Path& p)
{
	struct statfs st;
	if (statfs(p.c_str(), &st) < 0)
		throw Filesystem_error {p, errno};

	if (watchdog.native && st.f_type != NFS_SUPER_MAGIC)
		watchdog.native->add_path(p);
	else
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
			if (watchdog.native)
				watchdog.native->remove_path(p);

			if (watchdog.poll)
				watchdog.poll->remove_path(p);
		});
}

void start_filesystem_watchdog(std::chrono::milliseconds update_interval,
							   int enabled_watchdogs, On_fs_change&& fn)
{
	callbacks.on_fs_change = std::move(fn);

	if (enabled_watchdogs & WD_POLL)
	{
		auto poll_mon = std::make_unique<Poll_monitor>(update_interval);
		watchdog.poll = std::make_unique<Dir_watchdog>(
					std::move(poll_mon), &watchdog_notify);
	}

	if (enabled_watchdogs & WD_NATIVE)
	{
		// TODO: choose the right native Monitor type (depending on the system)
		// at compile-time.

		auto native_mon = std::make_unique<Inotify_monitor>(update_interval);
		watchdog.native = std::make_unique<Dir_watchdog>(
					std::move(native_mon), &watchdog_notify);
	}
}

void load_dir_ptr(Dir_ptr& ptr, const Path& directory, bool force_reload)
{
	ptr.reset();

	if (force_reload)
		storage->mark_dirty(directory);

	ptr = storage->get(directory);
	watch_directory(directory);
}

} // namespace hawk
