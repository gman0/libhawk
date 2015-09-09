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

#ifndef HAWK_POLL_MONITOR_H
#define HAWK_POLL_MONITOR_H

#include <vector>
#include <chrono>
#include "Path.h"
#include "dir-watchdog/Monitor.h"

namespace hawk {
	class Poll_monitor : public Monitor
	{
	private:
		struct Entry
		{
			Path path;
			time_t timestamp;

			Entry(const Path& p);
		};

		std::vector<Entry> m_entries;
		std::chrono::milliseconds m_delay;

	public:
		Poll_monitor(std::chrono::milliseconds delay)
			: m_delay{delay}
		{}

		virtual void add_path(const Path& dir);
		virtual void remove_path(const Path& dir);

		virtual void watch() noexcept;
	};
}

#endif // HAWK_POLL_MONITOR_H
