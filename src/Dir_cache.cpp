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
#include "Dir_cache.h"

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
	ptr->resize(max_size);
}

static class Cache_list
{
public:
	using Fn = std::function<void(Cache_entry& ent)>;

private:
	struct Node
	{
		std::atomic<bool> in_use;
		Node* next;
		Cache_entry ent;
	};

	std::atomic<Node*> m_head;
	std::mutex m_mutex;

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
		Node* n = new Node {{false}, nullptr, ent};
		n->next = m_head.load();

		// This is ok to do since we've got only single writer thread.
		m_head.store(n);
	}

	bool find(size_t path_hash, Cache_entry& out)
	{
		Node* n = m_head.load();
		while (n)
		{
			if (n->ent.hash == path_hash)
			{
				out = n->ent;
				return true;
			}

			n = n->next;
		}

		return false;
	}

	void for_each(Fn f)
	{
		std::lock_guard<std::mutex> lk {m_mutex};

		Node* n = m_head.load();
		while(n)
		{
			bool in_use = false;
			if (n->in_use.compare_exchange_strong(in_use, true,
								std::memory_order_release,
								std::memory_order_relaxed) && !in_use)
			{
				f(n->ent);
				n->in_use.store(false, std::memory_order_release);
			}

			n = n->next;
		}
	}

	bool try_allocate_free_ptr(Fn f)
	{
		Node* free = find_largest_vector();
		if (free)
		{
			spinlock_in_use(free);

			f(free->ent);

			free->in_use.store(false, std::memory_order_release);
		}

		return free;
	}

	void free_ptrs()
	{
		Node* n = m_head.load();
		while (n)
		{
			if (n->ent.ptr.use_count() == 1)
			{
				spinlock_in_use(n);

				n->ent.hash = 0;
				n->ent.timestamp = 0;

				shrink(n->ent.ptr);
				n->ent.ptr->clear();

				n->in_use.store(false, std::memory_order_release);
			}

			n = n->next;
		}
	}

	void delete_free_ptrs(int nptrs)
	{
		if (--nptrs > 0)
		{
			std::lock_guard<std::mutex> lk {m_mutex};

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
	Node* find_largest_vector()
	{
		Node* free = nullptr;
		Node* n = m_head.load();
		while(n)
		{
			if (is_free(n->ent))
			{
				if (!free)
					free = n;
				else
				{
					if (n->ent.ptr->capacity() > free->ent.ptr->capacity())
						free = n;
				}
			}

			n = n->next;
		}

		return free;
	}

	inline void spinlock_in_use(Node* n)
	{
		// Wait for the node to be released and mark it as
		// in-use rigt away.
		bool in_use = false;
		while (!n->in_use.compare_exchange_weak(in_use, true,
						std::memory_order_acquire,
						std::memory_order_relaxed) && in_use);
	}
} s_entries;

static struct Filesystem_watchdog
{
	std::unique_ptr<std::thread> thread;
	std::atomic<bool> done;
	On_fs_change_f on_change;

	Filesystem_watchdog() : done{false} {}
	~Filesystem_watchdog()
	{
		done.store(true);
		thread->join();
	}
} s_fs_watchdog;

// Gather contents of a directory and move them to the out vector
static void gather_dir_contents(const path& p, Dir_vector& out)
{
	directory_iterator dir_it_end;
	for (auto dir_it = directory_iterator {p};
		 dir_it != dir_it_end;
		 ++dir_it)
	{
		Dir_entry dir_ent;
		dir_ent.path = std::move(*dir_it);

		boost::system::error_code ec;
		dir_ent.status = status(dir_ent.path, ec);

		out.push_back(std::move(dir_ent));
	}
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
		// Ignore any exceptions as we're going to
		// reload_current_path later anyway.

		hash_vec.push_back(ent.hash);
	}
}

static void fs_watchdog()
{
	static std::vector<size_t> hash_vec;

	while (!s_fs_watchdog.done)
	{
		s_entries.for_each(
					[](Cache_entry& ent) {
			if (is_free(ent)) return;
			try_update_dir_contents(ent, hash_vec);
		});

		if (!hash_vec.empty())
		{
			try { s_fs_watchdog.on_change(hash_vec); }
			catch (...) {} // We can't deal with any exceptions at this point.

			hash_vec.clear();
		}

		if (!s_fs_watchdog.done.load())
			std::this_thread::sleep_for(std::chrono::seconds{1});
		else
			break;
	}
}

void start_filesystem_watchdog(On_fs_change_f&& on_fs_change)
{
	s_fs_watchdog.on_change = std::move(on_fs_change);
	s_fs_watchdog.thread.reset(new std::thread{&fs_watchdog});
}

static void build_cache_entry(Cache_entry& ent, const path& p,
							  size_t path_hash)
{
	ent.hash = path_hash;
	ent.timestamp = last_write_time(p);
	ent.path = p;
	ent.ptr = std::make_shared<Dir_vector>();
	gather_dir_contents(p, *ent.ptr);
}

Dir_ptr get_dir_ptr(const path& p, size_t path_hash)
{
	Cache_entry ent;

	if (s_entries.find(path_hash, ent))
		return ent.ptr;
	else
	{
		s_entries.free_ptrs();

		Cache_list::Fn update_entry = [&](Cache_entry& e) {
			e.hash = path_hash;
			e.timestamp = last_write_time(p);
			e.path = p;
			gather_dir_contents(p, *(e.ptr));

			ent = e;
		};

		if (s_entries.try_allocate_free_ptr(update_entry))
			return ent.ptr;
	}

	build_cache_entry(ent, p, path_hash);
	s_entries.push_front(ent);

	return ent.ptr;
}

void destroy_free_dir_ptrs(int free_ptrs)
{
	s_entries.delete_free_ptrs(free_ptrs);
}

} // namespace hawk
