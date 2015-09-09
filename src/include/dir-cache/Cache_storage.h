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

#ifndef HAWK_CACHE_STORAGE_H
#define HAWK_CACHE_STORAGE_H

#include <deque>
#include <mutex>
#include <functional>
#include "dir-cache/Dir_entry.h"
#include "dir-watchdog/Monitor.h"

namespace hawk {

	// Cache_storage stores Dir_ptr's and governs their life-times.
	// All methods are thread-safe.
	//
	/// Memory reusing
	//  Dir_ptr's are not deleted during Cache_storage's life-time,
	//  only their Dir_vector's ale cleared once the Dir_ptr's
	//  reference counter hits 1. This moves the particular Entry
	//  to a 'free' state. Upon next get() call it will be reaquired
	//  in case no other Entry fits the path requirement.
	/// Entries in dirty state
	//  When an Entry gets marked as 'dirty', it becomes a candidate
	//  for Dir_vector rebuild which means it will be re-populated
	//  (from populate_directory()) upon next get() call.
	class Cache_storage
	{
	public:
		using On_dir_ptr_free = std::function<void(const Path&)>;

	private:
		struct Entry
		{
			enum State { acquired, dirty, about_to_be_freed, free };

			Dir_ptr ptr;
			Path path;
			State st = free;
			std::mutex m;
		};

		using Entries = std::deque<Entry>;

		std::mutex m_entries_mtx;
		Entries m_entries;
		uint64_t m_free_threshold;

		On_dir_ptr_free m_on_free;

	public:
		Cache_storage(uint64_t free_threshold, On_dir_ptr_free&& on_ptr_free)
			:
			  m_free_threshold{free_threshold},
			  m_on_free{std::move(on_ptr_free)}
		{}

		Dir_ptr get(const Path& dir);
		void mark_dirty(const Path& p);

	private:
		Entry* find(
				const Path& p, std::unique_lock<std::mutex>& entry_lk);
		// Iterates through all entries, trying to free their Dir_ptr's.
		// Also tries to find an Entry with path `p'.
		Entry* free_and_find(
				const Path& p, std::unique_lock<std::mutex>& entry_lk);

		void try_free(Entry& ent);
		// Returns a pointer to reaquired Entry, which means we don't
		// need to insert another entry. Returns nullptr if no Entry
		// has been reaquired.
		Entry* try_reaquire(std::unique_lock<std::mutex>& entry_lk);

		Entry& insert_and_build(
				const Path& p, std::unique_lock<std::mutex>& entry_lk);
		void build_entry(Entry& ent, const Path& p);
	};
}

#endif // HAWK_CACHE_STORAGE_H
