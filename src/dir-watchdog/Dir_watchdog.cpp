#include <cassert>
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
	m_watchdog = Interruptible_thread([&]{
		for (;;)
		{
			hard_interruption_point();
			m_monitor->watch();
			process_queued_paths();
		}
	});
}

Dir_watchdog::~Dir_watchdog()
{
	m_watchdog.hard_interrupt();
}

void Dir_watchdog::add_path(const Path& dir)
{
	assert(is_directory(dir));
	std::lock_guard<std::mutex> lk {m_mtx};
	m_add_paths.push_back(dir);
}

void Dir_watchdog::remove_path(const Path& dir)
{
	std::lock_guard<std::mutex> lk {m_mtx};
	m_remove_paths.push_back(dir);
}

void Dir_watchdog::_notify(Monitor::Event e, const Path& p) noexcept
{
	m_notify(e, p);
}

void Dir_watchdog::process_queued_paths()
{
	std::lock_guard<std::mutex> lk {m_mtx};

	for (const Path& p : m_add_paths)
		m_monitor->add_path(p);

	for (const Path& p : m_remove_paths)
		m_monitor->remove_path(p);

	m_add_paths.clear();
	m_remove_paths.clear();
}

} // namespace hawk
