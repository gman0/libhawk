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

#ifndef HAWK_CACHE_H
#define HAWK_CACHE_H

#include <unordered_map>
#include <utility>
#include <functional>
#include <ctime>

namespace hawk {
	template <typename Key, typename T>
	class Cache
	{
	public:
		// time_t (timestamp) is used to determine whether we need
		// to update the cache
		using Cache_dictionary_entry = std::pair<std::time_t, T>;

		// ...in that case we'll update the cache by calling
		// this lambda
		using Update_lambda = std::function<void(T*)>;

	protected:
		T* m_active_cache;
		std::unordered_map<Key,
			Cache_dictionary_entry> m_cache_dictionary;

	private:
		Update_lambda m_update_closure;

	public:
		Cache(const Update_lambda& update_closure)
			: m_update_closure{update_closure} {}

		Cache(Update_lambda&& update_closure)
			: m_update_closure{std::move(update_closure)} {}

		Cache(Cache&& c)
			:
			m_active_cache{c.m_active_cache},
			m_cache_dictionary{std::move(c.m_cache_dictionary)}
		{ c.m_active_cache = nullptr; }

		Cache& operator=(const Cache&) = delete;
		Cache& operator=(Cache&& c);

		virtual ~Cache() {}

		void remove_active();
		T* switch_cache(std::time_t timestamp, const Key& k);
		T* get_active_cache() { return m_active_cache; }
		// void force_update_active();

	protected:
		virtual T* add_dir_entry(std::time_t timestamp, const Key& k);

	private:
		void update_cache(std::time_t timestamp,
			Cache_dictionary_entry& ent);
	};
}

#endif // HAWK_CACHE_H
