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

#include <cassert>
#include <algorithm>
#include "Filesystem.h"
#include "Path.h"
#include "dir-watchdog/Dir_watchdog.h"

namespace hawk
{

Dir_watchdog::Dir_watchdog(std::unique_ptr<Monitor>&& mon,
						   Dir_watchdog::Notify&& notify)
	:
	  m_monitor{std::move(mon)},
	  m_notify{std::move(notify)}
{
	m_monitor->set_parent(*this);
	m_watchdog = Interruptible_thread([this]{
		{
			std::unique_lock<std::mutex> lk {m_mtx};
			m_cv.wait(lk, [&]{ return !m_paths_to_add.empty(); });
		}

		for (;;)
		{
			hard_interruption_point();
			process_queued_paths();
			m_monitor->watch();
		}
	});
}

Dir_watchdog::~Dir_watchdog()
{
	m_watchdog.hard_interrupt();
	// Add a path in case m_watchdog is still waiting in m_cv.
	m_paths_to_add.emplace_back();
	m_cv.notify_one();
}

void Dir_watchdog::add_path(const Path& dir)
{
	assert(is_directory(dir));

	std::lock_guard<std::mutex> lk {m_mtx};
	m_paths_to_add.push_back(dir);
	m_cv.notify_one();
}

void Dir_watchdog::remove_path(const Path& dir)
{
	std::lock_guard<std::mutex> lk {m_mtx};
	m_paths_to_remove.push_back(dir);
}

void Dir_watchdog::_notify(Monitor::Event e, const Path& p) noexcept
{
	m_notify(e, p);
}

void Dir_watchdog::process_queued_paths()
{
	std::lock_guard<std::mutex> lk {m_mtx};
	add_paths();
	remove_paths();
}

void Dir_watchdog::add_paths()
{
	if (m_paths_to_add.empty())
		return;

	std::unique(m_paths_to_add.begin(), m_paths_to_add.end());
	for (const Path& p : m_paths_to_add)
		m_monitor->add_path(p);

	m_paths_to_add.clear();
}

void Dir_watchdog::remove_paths()
{
	if (m_paths_to_remove.empty())
		return;

	std::unique(m_paths_to_remove.begin(), m_paths_to_remove.end());
	for (const Path& p : m_paths_to_remove)
		m_monitor->remove_path(p);

	m_paths_to_remove.clear();
}

} // namespace hawk
