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

#ifndef HAWK_DIR_WATCHDOG_H
#define HAWK_DIR_WATCHDOG_H

#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "Interruptible_thread.h"
#include "dir-watchdog/Monitor.h"

namespace hawk {
	// Watches queued directories for modifications (adding/removing
	// a file/directory from the watched directory; removing the watched
	// directory itself). The watchdog is fed a Monitor which handles
	// the watching itself (this means that the frequency of watching
	// is determined by the Monitor). Watchdog creates a thread in
	// which the Monitor is executed.
	class Dir_watchdog
	{
	public:
		using Notify =
			std::function<void(Monitor::Event, const Path&) noexcept>;

	private:
		std::unique_ptr<Monitor> m_monitor;
		std::condition_variable m_cv;
		Interruptible_thread m_watchdog;
		Notify m_notify;

		std::mutex m_mtx;
		std::vector<Path> m_paths_to_add;
		std::vector<Path> m_paths_to_remove;

	public:
		Dir_watchdog(std::unique_ptr<Monitor>&& mon, Notify&& notify);
		~Dir_watchdog();

		void add_path(const Path& dir);
		void remove_path(const Path& dir);

		// Called internally by Monitors.
		void _notify(Monitor::Event e, const Path& p) noexcept;

	private:
		void process_queued_paths();
		void add_paths();
		void remove_paths();
	};
}

#endif // HAWK_DIR_WATCHDOG_H
