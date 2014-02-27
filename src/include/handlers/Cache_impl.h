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

#ifndef HAWK_CACHE_IMPL_H
#define HAWK_CACHE_IMPL_H

namespace hawk {
	template <typename Key, typename T>
	Cache<Key, T>& Cache<Key, T>::operator=(Cache&& c)
	{
		m_active_cache = c.m_active_cache;
		c.m_active_cache = nullptr;
		m_cache_dictionary = std::move(c.m_cache_dictionary);

		return *this;
	}

	template <typename Key, typename T>
	void Cache<Key, T>::remove_active()
	{
		for (auto& it : m_cache_dictionary)
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
	T* Cache<Key, T>::switch_cache(std::time_t timestamp, const Key& k)
	{
		if (m_cache_dictionary.find(k) == m_cache_dictionary.end())
		{
			m_active_cache = add_dir_entry(timestamp, k);
			m_update_closure(m_active_cache);

			return m_active_cache;
		}
		else
		{
			Cache_dictionary_entry& dir_ent = m_cache_dictionary[k];
			update_cache(timestamp, dir_ent);

			return (m_active_cache = &dir_ent.second);
		}
	}

	template <typename Key, typename T>
	T* Cache<Key, T>::add_dir_entry(std::time_t timestamp, const Key& k)
	{
		return &(m_cache_dictionary[k] = {timestamp, T()}).second;
	}

	template <typename Key, typename T>
	void Cache<Key, T>::update_cache(std::time_t timestamp,
		Cache::Cache_dictionary_entry& ent)
	{
		if (timestamp != ent.first)
		{
			m_update_closure(&ent.second);
			ent.first = timestamp;
		}
	}
}

#endif // HAWK_CACHE_IMPL
