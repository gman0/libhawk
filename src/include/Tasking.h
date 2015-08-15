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

#ifndef HAWK_TASKING_H
#define HAWK_TASKING_H

#include <vector>
#include <initializer_list>
#include <utility>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "Interruptible_thread.h"

namespace hawk {
	class Tasking
	{
	public:
		enum class Priority { low, high };

		using Task = std::function<void()>;
		using PTask = std::pair<Priority, Task>;

		using Exception_handler = std::function<
			void(std::exception_ptr) noexcept>;

	private:
		bool m_ready;
		Exception_handler m_eh;
		Priority m_current_priority;
		std::vector<std::pair<Priority, Task>> m_tasks;

		std::mutex m_mtx;
		std::condition_variable m_cv;
		Interruptible_thread m_thread;

	public:
		Tasking(const Exception_handler& eh)
			:
			  m_ready{true},
			  m_eh{eh},
			  m_current_priority{Priority::low},
			  m_thread{[this]{ run(); }}
		{}

		~Tasking();

		// A task will interrupt and run only if its priority is
		// is equal or higher than the priority of task currently being run.
		// In that case it returns true.
		bool run_task(Priority p, Task&& f);
		bool run_blocking_task(Priority p, Task&& f);

		// Runs tasks in succession (in forward order - from begin() to end()).
		// Returns false only if the first task could not be run because
		// of its low priority.
		bool run_tasks(std::initializer_list<PTask> l);

	private:
		void run();
		void dispatch_tasks();

		void start_tasks();
		void end_tasks();
	};
}

#endif // HAWK_TASKING_H
