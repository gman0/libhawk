#ifndef HAWK_DIR_WATCHDOG_H
#define HAWK_DIR_WATCHDOG_H

#include <memory>
#include <functional>
#include <vector>
#include <mutex>
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
		Interruptible_thread m_watchdog;
		Notify m_notify;

		std::mutex m_mtx;
		std::vector<Path> m_add_paths;
		std::vector<Path> m_remove_paths;

	public:
		Dir_watchdog(std::unique_ptr<Monitor>&& mon, Notify&& notify);
		~Dir_watchdog();

		void add_path(const Path& dir);
		void remove_path(const Path& dir);

		// Called internally by Monitors.
		void _notify(Monitor::Event e, const Path& p) noexcept;

	private:
		void process_queued_paths();
	};
}

#endif // HAWK_DIR_WATCHDOG_H
