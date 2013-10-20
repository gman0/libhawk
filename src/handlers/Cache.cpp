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

#include <exception>
#include "handlers/Cache.h"

using namespace hawk;

template <typename Key, typename T>
Cache<Key, T>::~Cache()
{
	for (auto it : m_cache_dictionary)
		delete it->second;
}

template <typename Key, typename T>
void Cache<Key, T>::remove_active()
{
	delete m_active_cache;

	// the cache dictionary is limited in size
	// so it's not that big of a deal if it
	// triggers rehashing of the unordered_map
	for (auto it : m_cache_dictionary)
	{
		if (it->second == m_active_cache)
		{
			m_cache_dictionary.erase(it);
			break;
		}
	}

	m_active_cache = nullptr;
}

template <typename Key, typename T>
T* Cache<Key, T>::switch_cache(const Key& k)
{
	if (m_cache_dictionary.find(k) == m_cache_dictionary.end())
	{
		auto insert_pair = m_cache_dictionary.insert(add_dir_entry(k));

		if (!insert_pair->second)
			throw std::runtime_error(
					"Cannot insert a new cache dictionary entry");

		return (m_active_cache = *(insert_pair->first));
	}
	else
		return (m_active_cache = m_cache_dictionary[k]);
}
