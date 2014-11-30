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

#ifndef HAWK_CURSOR_CACHE_H
#define HAWK_CURSOR_CACHE_H

#include <unordered_map>
#include "No_hash.h"

namespace hawk {
	class Cursor_cache
	{
	public:
		// The first size_t is the hash of the path to which
		// the cursor belongs to (key) and the second size_t
		// is the item in the directory to which the cursor
		// points to.
		using Map =
			std::unordered_map<size_t, size_t, No_hash>;

		using Cursor = Map::iterator;

	private:
		Map m_cursor_map;

	public:
		// Tries to find a cursor with key cursor_hash.
		// Returns true on success (that is result != m_cursor_map.end()).
		// The resulting find iterator is stored in iter.
		bool find(size_t cursor_hash, Cursor& iter);
		void store(size_t key, size_t cursor_hash);
	};
}

#endif // HAWK_CURSOR_CACHE_H
