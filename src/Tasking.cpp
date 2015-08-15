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

using Barriere = std::promise<void>;
class Barriere_guard
{
private:
	Barriere& m_barr;

public:
	Barriere_guard(Barriere& b) : m_barr{b} {}
	~Barriere_guard() { m_barr.set_value(); }
};

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

bool Tasking::run_task(Tasking::Priority p, Tasking::Task&& f)
{
	return run_tasks({ {p, std::move(f)} });
}

bool Tasking::run_tasks(std::initializer_list<PTask> l)
{
	std::unique_lock<std::mutex> lk {m_mtx};

	Priority first_p = l.begin()->first;
	if (first_p < m_current_priority)
		return false;

	m_thread.soft_interrupt();
	m_cv.wait(lk, [this]{ return m_ready; });

	m_ready = false;
	m_current_priority = first_p;
	m_tasks = l;

	lk.unlock();
	m_cv.notify_one();

	return true;
}

bool Tasking::run_blocking_task(Tasking::Priority p, Tasking::Task&& f)
{
	Barriere b;

	bool r = run_task(p, [&]{
		Barriere_guard bg {b};
		f();
	});

	if (r) b.get_future().wait();

	return r;
}

void Tasking::run()
{
	for (;;)
	{
		start_tasks();

		try { dispatch_tasks(); }
		catch (const Soft_thread_interrupt&) {}
		catch (const Hard_thread_interrupt&) { return; }
		catch (...) { m_eh(std::current_exception()); }

		end_tasks();
	}
}

void Tasking::dispatch_tasks()
{
	for (auto& pt : m_tasks)
	{
		{
			std::lock_guard<std::mutex> lk {m_mtx};
			m_current_priority = pt.first;
		}

		// Run the task.
		pt.second();
	}
}

void Tasking::start_tasks()
{
	std::unique_lock<std::mutex> lk {m_mtx};

	m_cv.wait(lk, [this]{ return !m_ready; });
	_soft_iflag.clear();

	m_cv.notify_one();
}

void Tasking::end_tasks()
{
	std::unique_lock<std::mutex> lk {m_mtx};

	m_ready = true;
	m_current_priority = Priority::low;
	m_tasks.clear();

	m_cv.notify_one();
}

} // namespace hawk
