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

#include "dir-cache/Cache_storage.h"

namespace hawk {

Dir_ptr Cache_storage::get(const Path& dir)
{
	std::unique_lock<std::mutex> lk;
	Entry* ent = free_and_find(dir, lk);

	if (!ent) ent = try_reaquire(lk);
	if (!ent) ent = &insert_and_build(dir, lk);

	if (ent->st != Entry::acquired)
		build_entry(*ent, dir);

	return ent->ptr;
}

void Cache_storage::mark_dirty(const Path& p)
{
	std::unique_lock<std::mutex> lk;
	Entry* ent = find(p, lk);

	if (ent) ent->st = Entry::dirty;
}

Cache_storage::Entry* Cache_storage::find(
		const Path& p, std::unique_lock<std::mutex>& entry_lk)
{
	std::lock_guard<std::mutex> lk {m_entries_mtx};

	for (Entry& e : m_entries)
	{
		std::unique_lock<std::mutex> llk {e.m};
		if (e.path == p)
		{
			entry_lk = std::move(llk);
			return &e;
		}
	}

	return nullptr;
}

Cache_storage::Entry* Cache_storage::free_and_find(
		const Path& p, std::unique_lock<std::mutex>& entry_lk)
{
	std::lock_guard<std::mutex> lk {m_entries_mtx};

	Entry* ent {};
	for (Entry& e : m_entries)
	{
		std::unique_lock<std::mutex> llk {e.m};

		if (e.path != p)
			try_free(e);
		else
		{
			entry_lk = std::move(llk);
			ent = &e;
		}
	}

	return ent;
}

void Cache_storage::try_free(Cache_storage::Entry& ent)
{
	if (!ent.ptr.unique())
		return;

	if (ent.st == Entry::acquired)
		ent.st = Entry::about_to_be_freed;
	else
	{
		ent.st = Entry::free;
		auto sz = ent.ptr->size();
		ent.ptr->clear();

		if (sz > m_free_threshold)
			ent.ptr->resize(m_free_threshold);

		m_on_free(ent.path);
	}
}

Cache_storage::Entry* Cache_storage::try_reaquire(
		std::unique_lock<std::mutex>& entry_lk)
{
	std::lock_guard<std::mutex> lk {m_entries_mtx};

	for (Entry& e : m_entries)
	{
		std::unique_lock<std::mutex> llk {e.m};
		if (e.st == Entry::free)
		{
			entry_lk = std::move(llk);
			return &e;
		}
	}

	return nullptr;
}

Cache_storage::Entry& Cache_storage::insert_and_build(
		const Path& p, std::unique_lock<std::mutex>& entry_lk)
{
	Entry* ent;

	{
		std::lock_guard<std::mutex> lk {m_entries_mtx};
		m_entries.emplace_back();
		ent = &m_entries.back();
		entry_lk = std::unique_lock<std::mutex> {ent->m};
		ent->ptr = std::make_unique<Dir_vector>();
	}

	build_entry(*ent, p);

	return *ent;
}

void Cache_storage::build_entry(Entry& ent, const Path& p)
{
	if (ent.st != Entry::about_to_be_freed)
	{
		ent.ptr->clear();
		populate_directory(*ent.ptr, p);
		ent.path = p;
	}

	ent.st = Entry::acquired;
}

} // namespace hawk
