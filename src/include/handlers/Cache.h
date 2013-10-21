/*
	Copyright (C) 2013 Róbert "gman" Vašek <gman@codefreax.org>

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

#ifndef HAWK_CACHE_H
#define HAWK_CACHE_H

#include <unordered_map>

namespace hawk {
	template <typename Key, typename T>
	class Cache
	{
	protected:
		T* m_active_cache;
		std::unordered_map<Key, T*> m_cache_dictionary;

	public:
		Cache& operator=(const Cache&) = delete;
		virtual ~Cache();

		virtual T* add_dir_entry(const Key& k) = 0;
		void remove_active();
		T* switch_cache(const Key& k);
		T* get_active_cache() { return m_active_cache; }
	};
}

#endif // HAWK_CACHE_H
