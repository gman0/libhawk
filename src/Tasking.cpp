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

#include <future>
#include "Tasking.h"

namespace hawk {

namespace {

bool operator<(Tasking::Priority lhs, Tasking::Priority rhs)
{
	return static_cast<int>(lhs) < static_cast<int>(rhs);
}

} // unnamed-namespace

Tasking::~Tasking()
{
	if (!m_thread.joinable())
		return;

	m_thread.hard_interrupt();
	run_task(Priority::high, []{
		for (;;) hard_interruption_point();
	});
}

void Tasking::run_task(Tasking::Priority p, Tasking::Task&& f)
{
	std::unique_lock<std::mutex> lk {m_mtx};

	if (p < m_task_pr)
	{
		// Don't interrupt tasks with higher priority!
		return;
	}

	m_thread.soft_interrupt();
	m_cv.wait(lk, [this]{ return m_ready; });

	m_ready = false;
	m_task_pr = p;
	m_task = std::move(f);

	lk.unlock();
	m_cv.notify_one();
}

void Tasking::run_blocking_task(Tasking::Priority p, Tasking::Task&& f)
{
	std::promise<void> pr;

	run_task(p, [&]{
		f();
		pr.set_value();
	});

	pr.get_future().wait();
}

void Tasking::run()
{
	std::unique_lock<std::mutex> lk {m_mtx, std::defer_lock};

	auto start_task = [&]{
		lk.lock();
		m_cv.wait(lk, [this]{ return !m_ready; });
		_soft_iflag.clear();
		lk.unlock();
		m_cv.notify_one();
	};

	auto end_task = [&]{
		lk.lock();
		m_ready = true;
		m_task_pr = Priority::low;
		lk.unlock();
		m_cv.notify_one();
	};

	for (;;)
	{
		start_task();

		try { m_task(); }
		catch (const Soft_thread_interrupt&) {}
		catch (const Hard_thread_interrupt&) { return; }
		catch (...) { m_eh(std::current_exception()); }

		end_task();
	}
}

} // namespace hawk
