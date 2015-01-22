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

#include <cstdio>
#include <forward_list>
#include <algorithm>
#include <cassert>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <fcntl.h>
#include "Filesystem.h"
#include "Interruptible_thread.h"
#include "IO_tasking.h"

#include <iostream>
namespace hawk {

static struct
{
	Exception_handler eh;
	Item_monitor imon;
	Progress_monitor pmon;
} s_callbacks;

void set_io_task_callbacks(Exception_handler&& eh, Item_monitor&& imon,
						   Progress_monitor&& pmon)
{
	s_callbacks.eh = std::move(eh);
	s_callbacks.imon = std::move(imon);
	s_callbacks.pmon = std::move(pmon);
}

class File
{
private:
	int m_fd;

public:
	File(const Path& file, int flags)
	{
		m_fd = open(file.c_str(), flags);
		if (m_fd == -1)
			throw Filesystem_error {file, errno};
	}

	~File() { close(m_fd); }

	int get_fd() const { return m_fd; }

	ssize_t read(char* buf, size_t sz)
	{
		return ::read(m_fd, buf, sz);
	}

	ssize_t write(char *buf, size_t sz)
	{
		return ::write(m_fd, buf, sz);
	}
};

static void update_progress(const Task_item& ti, size_t offset,
							std::chrono::steady_clock::time_point& last_update)
{
	auto now = std::chrono::steady_clock::now();
	if (now - last_update > std::chrono::seconds {1})
	{
		last_update = now;
		std::chrono::seconds time_elapsed =
				std::chrono::duration_cast<
				 std::chrono::seconds>(now - ti.start);

		Task_progress tp;
		tp.offset = offset;
		tp.rate = offset / time_elapsed.count();
		tp.eta_end = (ti.size - offset) * time_elapsed / offset;

		s_callbacks.pmon(tp);
	}
}

static void copy_file(const Path& src, const Path& dst)
{
	if (exists(dst))
		throw Filesystem_error {dst, EEXIST};

	File src_file {src, O_RDONLY};
	File dst_file {dst, O_WRONLY | O_CREAT};

	size_t sz = file_size(src);
	if (sz == 0)
		return;

	int alloc_status = posix_fallocate(dst_file.get_fd(), 0, sz);
	if (alloc_status != 0)
		throw Filesystem_error {dst, alloc_status};

	int adv_status = posix_fadvise(src_file.get_fd(), 0, sz,
								   POSIX_FADV_SEQUENTIAL);
	if (adv_status != 0)
		throw Filesystem_error {src, adv_status};

	constexpr int buf_size = 8192;
	char buf[buf_size];

	size_t offset = 0;

	auto last_update = std::chrono::steady_clock::now();
	Task_item ti = {src, dst, sz, last_update};
	s_callbacks.imon(ti);

	for (;;)
	{
		ssize_t read_sz = src_file.read(buf, buf_size);
		if (read_sz == 0)
			break;

		if (read_sz == -1)
			throw Filesystem_error {src, errno};

		if (dst_file.write(buf, read_sz) != read_sz)
			throw Filesystem_error {dst, errno};

		offset += read_sz;
		update_progress(ti, offset, last_update);

		soft_interruption_point();
	}
}

class IO_task
{
protected:
	Path m_src;
	Path m_dst;

public:
	IO_task(const Path& src, const Path& dst)
		: m_src{src}, m_dst{dst}
	{}

	virtual ~IO_task() = default;

	const Path& get_source() const { return m_src; }
	const Path& get_destination() const { return m_dst; }

	virtual void run() = 0;
};

class IO_task_copy : public IO_task
{
public:
	using IO_task::IO_task;

	virtual void run()
	{
		if (is_regular_file(m_src))
			copy_file(m_src, m_dst);
		else
			;
	}
};

class IO_task_move : public IO_task
{
public:
	using IO_task::IO_task;

	virtual void run()
	{
		std::this_thread::sleep_for(std::chrono::seconds {2});
		std::cout << "moving " << m_src.c_str()
				  << " -> " << m_dst.c_str()
				  << std::endl;
	}
};

static Interruptible_thread s_io_thread;
static class Task_queue
{
public:
	struct Item
	{
		std::unique_ptr<IO_task> task;
		std::atomic<bool> in_use;

		Item(std::unique_ptr<IO_task>&& t)
			: task{std::move(t)}, in_use{false}
		{}
	};

	using Iterator = std::forward_list<Item>::iterator;

private:
	std::forward_list<Item> m_queue;
	std::mutex m_mtx;

public:
	~Task_queue()
	{
		if (s_io_thread.joinable())
			s_io_thread.join();
	}

	void add(std::unique_ptr<IO_task>&& task)
	{
		std::unique_lock<std::mutex> lk {m_mtx};
		m_queue.emplace_front(std::move(task));
	}

	void erase(const Path& src, const Path& dest)
	{
		std::unique_lock<std::mutex> lk {m_mtx};

		auto before_it = m_queue.before_begin();
		auto pred = [&] (const Item& i) {
			++before_it;
			return src == i.task->get_source()
					&& dest == i.task->get_destination();
		};

		auto it = std::find_if(m_queue.begin(), m_queue.end(), pred);
		assert(it != m_queue.end() &&
				"This is either a bug or wrong paths were supplied");

		if (it->in_use)
		{
			s_io_thread.soft_interrupt();
			return;
		}

		bool flag = false;
		while (!it->in_use.compare_exchange_weak(flag, true) && !flag);

		m_queue.erase_after(before_it);
	}

	void erase(Iterator iter)
	{
		std::unique_lock<std::mutex> lk {m_mtx};

		Iterator prev = m_queue.before_begin();
		for (Iterator i = m_queue.begin(); i != m_queue.end(); ++i, ++prev)
		{
			if (i == iter)
				break;
		}

		m_queue.erase_after(prev);
	}

	// Returns false if the head is already in use (about to be erased).
	bool head(Iterator& front)
	{
		std::unique_lock<std::mutex> lk {m_mtx};

		// Calls to empty() and head() could lead to a race.
		// We need to check for empty() again here.
		if (m_queue.empty())
			return false;

		front = m_queue.begin();

		bool flag = false;
		return front->in_use.compare_exchange_strong(flag, true);
	}

	bool empty()
	{
		std::unique_lock<std::mutex> lk {m_mtx};
		return m_queue.empty();
	}
} s_tasks;

static void io_tasking()
{
	while (!s_tasks.empty())
	{
		Task_queue::Iterator item;
		if (!s_tasks.head(item))
			continue;

		try { item->task->run(); }
		catch (const Soft_thread_interrupt&) {}
		catch (...) { s_callbacks.eh(std::current_exception()); }

		s_tasks.erase(item);
	}
}

static void ready_io_thread()
{
	if (!s_io_thread.joinable())
		s_io_thread = Interruptible_thread {&io_tasking};
}

void copy(const Path& source, const Path& destination)
{
	s_tasks.add(std::make_unique<IO_task_copy>(source, destination));
	ready_io_thread();
}

void move(const Path& source, const Path& destination)
{
	s_tasks.add(std::make_unique<IO_task_move>(source, destination));
	ready_io_thread();
}

void cancel_task(const Path& source, const Path& destination)
{
	s_tasks.erase(source, destination);
}

} // namespace hawk
