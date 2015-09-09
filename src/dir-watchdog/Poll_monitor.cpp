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
#include "Filesystem.h"
#include "dir-watchdog/Dir_watchdog.h"
#include "dir-watchdog/Poll_monitor.h"

namespace hawk {

Poll_monitor::Entry::Entry(const Path& p)
	: path{p}, timestamp{last_write_time(p)}
{}

void Poll_monitor::add_path(const Path& dir)
{
	m_entries.emplace_back(dir);
}

void Poll_monitor::remove_path(const Path& dir)
{
	for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
	{
		if (it->path == dir)
		{
			m_entries.erase(it);
			break;
		}
	}
}

void Poll_monitor::watch() noexcept
{
	for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
	{
		int err;
		time_t current_timestamp = last_write_time(it->path, err);

		if (err)
		{
			m_watchdog->_notify(Event::deleted, it->path);
			m_entries.erase(it);
		}
		else if (it->timestamp < current_timestamp)
		{
			m_watchdog->_notify(Event::modified, it->path);
			it->timestamp = current_timestamp;
		}
	}

	std::this_thread::sleep_for(m_delay);
}

} // namespace hawk
