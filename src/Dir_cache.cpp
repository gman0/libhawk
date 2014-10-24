/*
	Copyright (C) 2013-2014 Róbert "gman" Vašek <gman@codefreax.org>

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
#include <boost/filesystem.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include "Dir_cache.h"
#include "Interrupt.h"

using namespace boost::filesystem;

namespace hawk {

struct Cache_entry
{
	size_t hash;
	time_t timestamp;
	boost::filesystem::path path;
	Dir_ptr ptr;
};

static bool is_free(const Cache_entry& ent) { return ent.hash == 0; }

// Don't waste space.
static void shrink(Dir_ptr& ptr)
{
	constexpr unsigned max_size = 1024;

	if (ptr->size() > max_size)
		ptr->resize(max_size);
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

	void push_front(const Cache_entry& ent)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		Node* n = new Node {{Node_state::not_in_use}, nullptr, ent};
		n->next = m_head.load();

		while (!m_head.compare_exchange_weak(n->next, n));
	}

	Dir_ptr find(size_t path_hash)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		Node* n = m_head.load();
		while (n)
		{
			if (n->ent.hash == path_hash
				&& n->state.load() != Node_state::in_use_skip)
			{
				if (wait_for_state(n->state, Node_state::in_use_block,
								   Node_state::in_use_skip))
				{
					// 2nd check because some other thread could have
					// modified this before setting the state to in_use_block.
					if (n->ent.hash == path_hash)
					{
						Dir_ptr ptr = n->ent.ptr;
						n->state.store(Node_state::not_in_use);

						return ptr;
					}

					n->state.store(Node_state::not_in_use);
				}
			}

			n = n->next;
		}

		return nullptr;
	}

	void for_each(Fn f)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		Node* n = m_head.load();
		while(n)
		{
			if (n->state.load() != Node_state::in_use_skip)
			{
				if (wait_for_state(n->state, Node_state::in_use_block,
								   Node_state::in_use_skip))
				{
					f(n->ent);
					n->state.store(Node_state::not_in_use);
				}
			}

			n = n->next;
		}
	}

	bool try_reclaim_free_ptr(Fn f)
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		Node* free = nullptr;
		Node* n = m_head.load();
		while (n)
		{
			if (wait_for_state(n->state, Node_state::in_use_skip,
							   Node_state::in_use_block))
			{
				if (!is_free(n->ent))
					n->state.store(Node_state::not_in_use);
				else
				{
					if (!free) free = n;
					else
					{
						if (n->ent.ptr->capacity() > free->ent.ptr->capacity())
						{
							free->state.store(Node_state::not_in_use);
							free = n;
						}
					}
				}

				n = n->next;
			}
		}

		if (free)
			f(free->ent);

		return free;
	}

	void free_ptrs()
	{
		std::shared_lock<std::shared_timed_mutex> lk {m_smutex};

		Node* n = m_head.load();
		while (n)
		{
			Node_state st = Node_state::not_in_use;
			if (n->state.compare_exchange_strong(st, Node_state::in_use_skip))
			{
				if (is_free(n->ent))
				{
					n->ent.hash = 0;
					shrink(n->ent.ptr);
					n->ent.ptr->clear();
				}

				n->state.store(Node_state::not_in_use);
			}

			n = n->next;
		}
	}

	void delete_free_ptrs(int nptrs)
	{
		if (--nptrs > 0)
		{
			std::lock_guard<std::shared_timed_mutex> lk {m_smutex};

			Node* prev = nullptr;
			Node* n = m_head.load();
			while (n && nptrs >= 0)
			{
				Node* next = n->next;

				if (is_free(n->ent))
				{
					delete n;

					if (prev)
						prev->next = next;
				}
				else
					prev = n;

				n = next;
			}
		}
	}

private:
	bool wait_for_state(std::atomic<Node_state>& state, Node_state desired,
						Node_state fail)
	{
		bool cont = true;
		Node_state st = Node_state::not_in_use;
		while (!state.compare_exchange_weak(st, desired)
			   && st != Node_state::not_in_use)
		{
			if (st == fail)
			{
				cont = false;
				break;
			}
		}

		return cont;
	}
};

struct Filesystem_watchdog
{
	std::thread thread;
	std::atomic<bool> done;
	On_fs_change_f on_change;

	Filesystem_watchdog() : done{false} {}
	~Filesystem_watchdog()
	{
		done.store(true);
		thread.join();
	}
};

static struct
{
	Cache_list entries;
	Filesystem_watchdog fs_watchdog;

	On_sort_change_f on_sort_change;
	Dir_sort_predicate sort_pred;
	Populate_user_data populate_user_data;
} s_state;

static void sort_dir(Dir_vector& vec)
{
	if (s_state.sort_pred)
		std::sort(vec.begin(), vec.end(), s_state.sort_pred);
}

static void populate_user_data(Dir_entry& ent)
{
	if (s_state.populate_user_data)
		s_state.populate_user_data(ent);
}

// Gather contents of a directory and move them to the out vector
static void gather_dir_contents(const path& p, Dir_vector& out)
{
	directory_iterator dir_it_end;
	for (auto dir_it = directory_iterator {p};
		 dir_it != dir_it_end;
		 ++dir_it)
	{
		soft_interruption_point();

		Dir_entry dir_ent;
		dir_ent.path = std::move(*dir_it);
		populate_user_data(dir_ent);

		soft_interruption_point();

		out.push_back(std::move(dir_ent));
	}

	sort_dir(out);
}

static void try_update_dir_contents(Cache_entry& ent,
									std::vector<size_t>& hash_vec)
{
	if (!exists(ent.path))
	{
		hash_vec.push_back(ent.hash);
		return;
	}

	time_t new_timestamp = last_write_time(ent.path);

	if (new_timestamp > ent.timestamp)
	{
		ent.ptr->clear();
		ent.timestamp = new_timestamp;

		try { gather_dir_contents(ent.path, *(ent.ptr)); }
		catch (const filesystem_error&) {}
		// Ignore any filesystem exceptions as we're
		// going to reload_current_path later anyway.

		hash_vec.push_back(ent.hash);
	}
}

static void fs_watchdog()
{
	std::vector<size_t> hash_vec;

	while (!s_state.fs_watchdog.done)
	{
		s_state.entries.for_each(
			[&hash_vec](Cache_entry& ent) {
				if (is_free(ent)) return;
				try_update_dir_contents(ent, hash_vec);
		});

		if (!hash_vec.empty())
		{
			try { s_state.fs_watchdog.on_change(hash_vec); }
			catch (...) {} // We can't deal with any exceptions at this point.

			hash_vec.clear();
		}

		if (!s_state.fs_watchdog.done)
			std::this_thread::sleep_for(std::chrono::seconds {1});
		else
			break;
	}
}

void _start_filesystem_watchdog(On_fs_change_f&& on_fs_change,
							   On_sort_change_f&& on_sort_change,
							   Populate_user_data&& populate)
{
	static bool call_once = false;
	if (!call_once) call_once = true;
	else assert("hawk::_start_filesystem_watchdog can be called only once!");


	s_state.fs_watchdog.on_change = std::move(on_fs_change);
	s_state.on_sort_change = std::move(on_sort_change);
	s_state.populate_user_data = std::move(populate);
	s_state.fs_watchdog.thread = std::thread {&fs_watchdog};
}

void set_sort_predicate(Dir_sort_predicate&& pred)
{
	s_state.sort_pred = std::move(pred);

	s_state.entries.for_each([](Cache_entry& ent) {
		sort_dir(*ent.ptr);
	});

	s_state.on_sort_change();
}

static void build_cache_entry(Cache_entry& ent, const path& p,
							  size_t path_hash,
							  std::shared_ptr<Dir_vector>&& ptr)
{
	ent.hash = path_hash;
	ent.timestamp = last_write_time(p);
	ent.path = p;
	ent.ptr = std::move(ptr);
	gather_dir_contents(p, *ent.ptr);
}

Dir_ptr get_dir_ptr(const path& p, size_t path_hash)
{
	Cache_entry ent;

	try
	{
		Dir_ptr ptr;
		if ((ptr = s_state.entries.find(path_hash)))
			return ptr;
		else
		{
			soft_interruption_point();

			s_state.entries.free_ptrs();

			Cache_list::Fn update_entry = [&](Cache_entry& e) {
				build_cache_entry(e, p, path_hash, std::move(e.ptr));
				ptr = e.ptr;
			};

			soft_interruption_point();

			if (s_state.entries.try_reclaim_free_ptr(update_entry))
				return ptr;
		}

		build_cache_entry(ent, p, path_hash, std::make_shared<Dir_vector>());
	}
	catch (const Soft_thread_interrupt&)
	{
		return nullptr;
	}

	s_state.entries.push_front(ent);

	return ent.ptr;
}

void destroy_free_dir_ptrs(int free_ptrs)
{
	s_state.entries.delete_free_ptrs(free_ptrs);
}

} // namespace hawk
