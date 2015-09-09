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

#ifndef HAWK_INOTIFY_MONITOR_H
#define HAWK_INOTIFY_MONITOR_H

#include <unordered_map>
#include <chrono>
#include <poll.h>
#include "Path.h"
#include "dir-watchdog/Monitor.h"

struct inotify_event;

namespace hawk {
	// Inotify_monitor watches for modifications in directories (adding/removing
	// a file/direcotry) as well as modifications in files (e.g. write, truncate).

	// A known 'bug': Monitor::Event notifications are broadcasted only when
	// inotify has no events to report (i.e. when poll has timed-out). This
	// saves up for a lot of unnecessary updates. It also means notifications
	// might or might not get to the user in a timely manner. For example,
	// IO_tasking.cpp's copy_file will trigger Inotify_monitor (e.g. for
	// for updating the file size), but the user might recieve the
	// actual notification after the file has been copied OR during
	// the copying -- this depends on the time spent between the write()
	// calls and the value of timeout.
	// I will fix this if it causes problems...

	class Inotify_monitor : public Monitor
	{
	private:
		struct Watch
		{
			Path path;
			Event ev;
		};

		using Map = std::unordered_map<int, Watch>;

		int m_ifd; // inotify fd
		pollfd m_pfd; // poll fd

		// int: inotify's wd
		std::unordered_map<int, Watch> m_watches;
		std::chrono::milliseconds m_timeout;

	public:
		Inotify_monitor(std::chrono::milliseconds timeout);
		~Inotify_monitor();

		virtual void add_path(const Path& dir);
		virtual void remove_path(const Path& dir);

		virtual void watch() noexcept;

	private:
		bool contains_path(const Path& p) const;
		bool contains_path(const Path& p, Map::iterator& iter);

		ssize_t read_ifd(char* buf, int len);
		void batch_events(const inotify_event* beg, const void* end);
		void broadcast_notifications();
	};
}

#endif // HAWK_INOTIFY_MONITOR_H
