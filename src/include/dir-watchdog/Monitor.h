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

#ifndef HAWK_MONITOR_H
#define HAWK_MONITOR_H

namespace hawk {
	class Dir_watchdog;
	class Path;

	class Monitor
	{
	public:
		// We're only interested in modifications to the watched directory or
		// the watched directory itself was deleted/moved (Event::deleted).
		enum class Event { modified, deleted, none };

	protected:
		Dir_watchdog* m_watchdog;

	public:
		Monitor() : m_watchdog{nullptr} {}

		Monitor(const Monitor&) = delete;
		Monitor& operator=(const Monitor&) = delete;

		// Monitors have to remove their existing watches upon exiting.
		virtual ~Monitor() = default;

		void set_parent(Dir_watchdog& wdog) { m_watchdog = &wdog; }

		// It's guaranteed that add_path and remove_path won't be called
		// at the same time.
		virtual void add_path(const Path& dir) = 0;
		virtual void remove_path(const Path& dir) = 0;

		// A single pass of watching the queued directories.
		// Deleted directories have to be removed from the directory queue.
		// It's guaranteed that no add_path/remove_path calls will be made
		// by other threads during the execution of watch().
		virtual void watch() noexcept = 0;
	};
}

#endif // HAWK_MONITOR_H
