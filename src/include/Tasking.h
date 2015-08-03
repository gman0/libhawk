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

#include <utility>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "Interruptible_thread.h"

namespace hawk {
	class Tasking
	{
	public:
		using Task = std::function<void()>;
		enum class Priority { low, high };

		using Exception_handler = std::function<
			void(std::exception_ptr) noexcept>;

	private:
		bool m_ready;
		Priority m_task_pr;
		Exception_handler m_eh;
		Task m_task;

		std::mutex m_mtx;
		std::condition_variable m_cv;
		Interruptible_thread m_thread;

	public:
		Tasking(const Exception_handler& eh)
			:
			  m_ready{true},
			  m_task_pr{Priority::low},
			  m_eh{eh},
			  m_thread{[this]{ run(); }}
		{}

		~Tasking();

		// A task will interrupt and run only if its priority is
		// is equal or higher than the priority of task currently being run.
		void run_task(Priority p, Task&& f);
		void run_blocking_task(Priority p, Task&& f);

	private:
		void run();
	};
}

#endif // HAWK_TASKING_H
