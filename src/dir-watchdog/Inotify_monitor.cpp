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

#include <system_error>
#include <cerrno>
#include <cassert>
#include <climits>
#include <unistd.h>
#include <sys/inotify.h>
#include "Filesystem.h"
#include "dir-watchdog/Dir_watchdog.h"
#include "dir-watchdog/Inotify_monitor.h"

namespace hawk {

Inotify_monitor::Inotify_monitor(std::chrono::milliseconds timeout)
	: m_timeout{timeout}
{
	m_ifd = inotify_init();
	if (m_ifd < 0)
	{
		throw std::system_error
			{errno, std::system_category(), "Could not initialize inotify"};
	}

	m_pfd.fd = m_ifd;
	m_pfd.events = POLLIN;
	m_pfd.revents = 0;
}

Inotify_monitor::~Inotify_monitor()
{
	for (auto it = m_watches.begin(); it != m_watches.end(); ++it)
		inotify_rm_watch(m_ifd, it->first);

	close(m_ifd);
}

void Inotify_monitor::add_path(const Path& dir)
{
	assert(is_directory(dir));

	for (auto& pair : m_watches)
	{
		// Skip if this path is already watched.
		if (pair.second.path == dir)
			return;
	}

	int wd = inotify_add_watch(m_ifd, dir.c_str(),
							   IN_CREATE | IN_DELETE | IN_DELETE_SELF
							   | IN_MOVE | IN_MODIFY);
	if (wd < 0)
	{
		throw std::system_error
			{errno, std::system_category(), "inotify_add_watch failed"};
	}

	m_watches[wd] = {dir, Event::none};
}

void Inotify_monitor::remove_path(const Path& dir)
{
	Map::iterator it;
	if (!contains_path(dir, it))
		return;

	int wd = it->first;
	m_watches.erase(it);

	if (inotify_rm_watch(m_ifd, wd) < 0 && errno != EINVAL)
	{
		throw std::system_error
			{errno, std::system_category(), "inotify_rm_watch failed"};
	}
}

void Inotify_monitor::watch() noexcept
{
	int ready = poll(&m_pfd, 1, m_timeout.count());

	if (ready < 0)
		throw std::system_error {errno, std::system_category(), "poll failed"};
	if (ready == 0)
	{
		broadcast_notifications();
		return;
	}

	alignas(inotify_event) char buf[4096];
	ssize_t len = read_ifd(buf, sizeof(buf));
	batch_events(reinterpret_cast<const inotify_event*>(buf), buf + len);
}

bool Inotify_monitor::contains_path(const Path& p) const
{
	for (const auto& pair : m_watches)
	{
		if (pair.second.path == p)
			return true;
	}

	return false;
}

bool Inotify_monitor::contains_path(
		const Path& p, Map::iterator& iter)
{
	auto it = m_watches.begin();
	for (; it != m_watches.end(); ++it)
	{
		if (it->second.path == p)
		{
			iter = it;
			return true;
		}
	}

	return false;
}

ssize_t Inotify_monitor::read_ifd(char* buf, int len)
{
	ssize_t n = read(m_ifd, buf, len);
	if (n < 0)
	{
		throw std::system_error
			{errno, std::system_category(), "Error while reading from inotify file descriptor"};
	}

	return n;
}

void Inotify_monitor::batch_events(const inotify_event* beg, const void* end)
{
	for (const inotify_event* ev = beg; ev < end;
		 ev += sizeof(inotify_event) + ev->len)
	{
		if (ev->mask & IN_IGNORED)
			continue;

		if (ev->mask & IN_DELETE_SELF)
		{
			m_watches[ev->wd].ev = Event::deleted;

			// inotify might have skipped IN_DELETE event,
			// we have to search for a possible change manually
			// (that is if this directory has been deleted (IN_DELETE_SELF),
			// that means its parent directory has been modified as well).
			Map::iterator it;
			if (contains_path(m_watches[ev->wd].path.parent_path(), it))
				m_watches[it->first].ev = Event::modified;
		}
		else
			m_watches[ev->wd].ev = Event::modified;
	}
}

void Inotify_monitor::broadcast_notifications()
{
	for (auto& pair : m_watches)
	{
		if (pair.second.ev != Event::none)
		{
			m_watchdog->_notify(pair.second.ev, pair.second.path);
			pair.second.ev = Event::none;
		}
	}
}

} // namespace hawk
