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

#include <utility>
#include <functional>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <future>
#include <algorithm>
#include "Dir_cache.h"
#include "Interrupt.h"
#include "Filesystem.h"

namespace hawk {

namespace {

struct Cache_entry
{
	Dir_ptr ptr;
	Path path;
	bool about_to_be_freed;
	std::atomic<bool> dirty;
	time_t timestamp;

	Cache_entry() {}
	Cache_entry(const Cache_entry& e)
		: ptr{e.ptr}, path{e.path}, about_to_be_freed{e.about_to_be_freed},
		  dirty{e.dirty.load()}, timestamp{e.timestamp}
	{}

	Cache_entry(Cache_entry&& e)
		: ptr{std::move(e.ptr)}, path{std::move(e.path)},
		  about_to_be_freed{e.about_to_be_freed},
		  dirty{e.dirty.load()}, timestamp{e.timestamp}
	{}
};

// Don't waste space.
void shrink(Dir_ptr& ptr)
{
	constexpr unsigned max_size = 1024;

	if (ptr->size() > max_size)
		ptr->resize(max_size);
}

bool is_free(const Cache_entry& ent)
{
	return ent.path.empty();
}

void free_ptr(Cache_entry& ent)
{
	ent.path.clear();
	shrink(ent.ptr);
	ent.ptr->clear();
}

void check_and_free(Cache_entry& ent)
{
	if (ent.ptr.unique())
	{
		if (ent.about_to_be_freed) free_ptr(ent);
		else ent.about_to_be_freed = true;
	}
}

class Cache_list
{
public:
	using Fn = std::function<void(Cache_entry&)>;

private:
	enum class Node_state : char
	{
		not_in_use,
		in_use_block,
		in_use_skip
	};

	struct Node
	{
		std::atomic<Node_state> state;
		Node* next;
		Cache_entry ent;
	};

	std::atomic<Node*> m_head;
	std::shared_timed_mutex m_smutex;

public:

	class Node_handle
	{
	private:
		Node* m_node;

	public:
		Node_handle() : m_node{nullptr} {}
		Node_handle(Node* node) : m_node{node} {}
		~Node_handle()
		{
			if (m_node)
				m_node->state.store(Node_state::not_in_use);
		}

		Cache_entry& get() { return m_node->ent; }
	};

	Cache_list() : m_head{nullptr} {}
	~Cache_list()
	{
		Node* n = m_head.load();
		while (n)
		{
			Node* tmp = n;
			n = n->next;

			delete tmp;
		}
	}

	Dir_ptr push_front(Cache_entry&& ent)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		Node* n = new Node {{Node_state::not_in_use}, nullptr, std::move(ent)};
		n->next = m_head.load();

		while (!m_head.compare_exchange_weak(n->next, n));

		return n->ent.ptr;
	}

	bool find(size_t path_hash, Node_handle& nh)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		for (Node* n = m_head.load(); n != nullptr; n = n->next)
		{
			if (n->ent.path.hash() == path_hash
				&& n->state.load() != Node_state::in_use_skip
				&& wait_for_state(n->state, Node_state::in_use_block,
								  Node_state::in_use_skip))
			{
				// 2nd check because some other thread could have
				// modified this before setting the state to in_use_block.
				if (n->ent.path.hash() == path_hash)
				{
					nh = Node_handle {n};
					return true;
				}

				n->state.store(Node_state::not_in_use);
			}
		}

		return false;
	}

	void for_each(Fn f)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		for (Node* n = m_head.load(); n != nullptr; n = n->next)
		{
			if (is_free(n->ent)) continue;
			if (wait_for_state(n->state, Node_state::in_use_block,
							   Node_state::in_use_skip))
			{
				if (is_free(n->ent)) continue;
				f(n->ent);
				n->state.store(Node_state::not_in_use);
			}
		}
	}

	bool try_reclaim_free_ptr(Fn f)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		Node* free = nullptr;

		for (Node* n = m_head.load(); n != nullptr; n = n->next)
		{
			if (!wait_for_state(n->state, Node_state::in_use_skip,
								Node_state::in_use_block))
				continue;

			if (!is_free(n->ent))
				n->state.store(Node_state::not_in_use);
			else
			{
				if (!free) free = n;
				else if (n->ent.ptr->capacity() > free->ent.ptr->capacity())
				{
					free->state.store(Node_state::not_in_use);
					free = n;
				}
			}
		}

		if (free)
		{
			f(free->ent);
			free->state.store(Node_state::not_in_use);
		}

		return free;
	}

	void free_ptrs()
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		for (Node* n = m_head.load(); n != nullptr; n = n->next)
		{
			Node_state st = Node_state::not_in_use;
			if (n->state.compare_exchange_strong(st, Node_state::in_use_skip))
			{
				check_and_free(n->ent);
				n->state.store(Node_state::not_in_use);
			}
		}
	}

	void delete_free_ptrs(int nptrs)
	{
		if (--nptrs <= 0)
			return;

		std::lock_guard<std::shared_timed_mutex> lk {m_smutex};

		Node* prev = nullptr;
		Node* n = m_head.load();
		while (n && nptrs >= 0)
		{
			Node* next = n->next;

			if (is_free(n->ent))
			{
				delete n;
				nptrs--;

				if (prev) prev->next = next;
				else m_head = next;
			}
			else
				prev = n;

			n = next;
		}
	}

private:
	bool wait_for_state(std::atomic<Node_state>& state, Node_state desired,
						Node_state fail)
	{
		Node_state st = Node_state::not_in_use;
		while (!state.compare_exchange_weak(st, desired)
			   && st != Node_state::not_in_use)
		{
			if (st == fail)
				return false;
		}

		return true;
	}
};

struct Filesystem_watchdog
{
	std::thread thread;
	std::atomic<bool> done;
	On_fs_change on_change;

	Filesystem_watchdog() : done{false} {}
	~Filesystem_watchdog()
	{
		done.store(true);

		if (thread.joinable())
			thread.join();
	}
};

struct
{
	Cache_list entries;
	Filesystem_watchdog fs_watchdog;
	std::chrono::milliseconds update_interval;

	std::mutex sort_pred_mtx;

	On_sort_change on_sort_change;
	Dir_sort_predicate sort_pred;
	Populate_user_data populate_user_data;
} s_state;


constexpr int sort_granularity = 10000;

void parallel_sort(size_t beg, size_t end, Dir_vector& vec)
{
	Dir_vector::iterator start_it = vec.begin();

	size_t d = end - beg;
	if (d < sort_granularity)
		std::sort(start_it + beg, start_it + end, s_state.sort_pred);
	else
	{
		const size_t sz = vec.size();
		size_t next_beg = end;
		size_t next_end = next_beg + sort_granularity;

		if (next_end > sz)
			next_end = sz;

		auto beg_it = start_it + beg;
		auto next_beg_it = start_it + next_beg;
		auto next_end_it = start_it + next_end;

		soft_interruption_point();

		auto f = std::async(std::launch::async, parallel_sort, next_beg,
							next_end, std::ref(vec));
		std::sort(beg_it, start_it + end, s_state.sort_pred);
		f.wait();

		std::inplace_merge(beg_it, next_beg_it,
						   next_end_it, s_state.sort_pred);
	}
}

void sort_dir(Dir_vector& v)
{
	if (s_state.sort_pred)
	{
		std::lock_guard<std::mutex> lk {s_state.sort_pred_mtx};
		parallel_sort(0, v.size(), v);
	}
}

// Gather contents of a directory and move them to the out vector
void gather_dir_contents(const Path& p, Dir_vector& out)
{
	Directory_iterator dir_it_end;
	for (auto dir_it = Directory_iterator {p};
		 dir_it != dir_it_end;
		 ++dir_it)
	{
		soft_interruption_point();

		Dir_entry dir_ent;
		dir_ent.path = *dir_it;
		populate_user_data(p, dir_ent);

		out.push_back(std::move(dir_ent));
	}

	sort_dir(out);
}

// Marks a Dir_ptr as dirty if its timestamp is outdated.
void mark_outdated_ptr_dirty(
		Cache_entry& ent, std::vector<size_t>& hash_vec)
{
	if (!exists(ent.path))
	{
		check_and_free(ent);
		return;
	}

	int err;
	time_t new_timestamp = last_write_time(ent.path, err);

	if (new_timestamp > ent.timestamp)
	{
		ent.dirty.store(true, std::memory_order_release);
		hash_vec.push_back(ent.path.hash());
	}
}

void fs_watchdog()
{
	std::vector<size_t> hash_vec;

	while (!s_state.fs_watchdog.done)
	{
		s_state.entries.for_each([&hash_vec](Cache_entry& ent) {
			mark_outdated_ptr_dirty(ent, hash_vec);
		});

		if (!hash_vec.empty())
		{
			try { s_state.fs_watchdog.on_change(hash_vec); }
			catch (...) {} // We can't deal with any exceptions at this point.

			hash_vec.clear();
		}

		if (!s_state.fs_watchdog.done)
			std::this_thread::sleep_for(s_state.update_interval);
		else
			break;
	}
}

void build_cache_entry(Cache_entry& ent, const Path& p,
					   std::shared_ptr<Dir_vector>&& ptr)
{
	ent.about_to_be_freed = false;
	ent.dirty = false;
	ent.timestamp = last_write_time(p);
	ent.path = p;
	ent.ptr = std::move(ptr);

	gather_dir_contents(p, *ent.ptr);
}

void rebuild_cache_entry(Cache_entry& ent)
{
	ent.about_to_be_freed = false;
	ent.dirty = false;
	ent.timestamp = last_write_time(ent.path);
	ent.ptr->clear();

	gather_dir_contents(ent.path, *ent.ptr);
}

} // unnamed-namespace


void populate_user_data(const Path& base, Dir_entry& ent)
{
	if (s_state.populate_user_data)
		s_state.populate_user_data({base / ent.path}, ent.user_data);
}

void start_filesystem_watchdog(std::chrono::milliseconds update_interval,
							   On_fs_change&& on_fs_change)
{
	static bool call_once = false;
	if (!call_once) call_once = true;
	else assert(0 && "start_filesystem_watchdog can be called only once!");

	s_state.update_interval = update_interval;
	s_state.fs_watchdog.on_change = std::move(on_fs_change);
	s_state.fs_watchdog.thread = std::thread {&fs_watchdog};
}

void set_on_sort_change(On_sort_change&& on_sort_change)
{
	assert(!s_state.fs_watchdog.thread.joinable()
		   && "Setting On_sort_change is not allowed while filesystem-wachdog is running");
	s_state.on_sort_change = std::move(on_sort_change);
}

void set_populate_user_data(Populate_user_data&& populate_ud)
{
	assert(!s_state.fs_watchdog.thread.joinable()
		   && "Setting Populate_user_data is not allowed while filesystem-wachdog is running");
	s_state.populate_user_data = std::move(populate_ud);
}

void set_sort_predicate(Dir_sort_predicate&& pred)
{
	std::lock_guard<std::mutex> lk {s_state.sort_pred_mtx};

	s_state.sort_pred = std::move(pred);

	s_state.entries.for_each([](Cache_entry& ent) {
		sort_dir(*ent.ptr);
	});

	s_state.on_sort_change();
}

Dir_sort_predicate get_sort_predicate()
{
	return s_state.sort_pred;
}

void load_dir_ptr(Dir_ptr& ptr, const Path& p)
{
	ptr.reset();

	Cache_list::Node_handle nh;
	if (s_state.entries.find(p.hash(), nh))
	{
		Cache_entry& ent = nh.get();
		if (ent.dirty.load(std::memory_order_acquire))
			rebuild_cache_entry(ent);

		ptr = ent.ptr;
		return;
	}
	else
	{
		s_state.entries.free_ptrs();
		soft_interruption_point();

		Cache_list::Fn update_entry = [&](Cache_entry& e) {
			build_cache_entry(e, p, std::move(e.ptr));
			ptr = e.ptr;
		};

		if (s_state.entries.try_reclaim_free_ptr(update_entry))
			return;
	}

	Cache_entry ent;
	build_cache_entry(ent, p, std::make_shared<Dir_vector>());

	ptr = ent.ptr;
	s_state.entries.push_front(std::move(ent));
}

void destroy_free_dir_ptrs(int free_ptrs)
{
	s_state.entries.delete_free_ptrs(free_ptrs);
}

} // namespace hawk
