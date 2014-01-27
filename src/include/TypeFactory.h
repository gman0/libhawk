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

#ifndef HAWK_TYPE_FACTORY_H
#define HAWK_TYPE_FACTORY_H

#include <unordered_map>
#include <functional>
#include <vector>
#include <utility>
#include <boost/filesystem/path.hpp>
#include <magic.h>
#include "NoHash.h"
#include "Column.h"

namespace hawk {
	class Handler;

	class Type_factory
	{
	public:
		using Type_product =
			std::function<Handler*(const boost::filesystem::path&,
									Column*)>;

	private:
		using Type_map =
			std::unordered_map<size_t, Type_product, No_hash>;

		Type_map m_types;

		struct Magic_guard
		{
			magic_t magic_cookie;

			Magic_guard();
			~Magic_guard();
		} m_magic_guard;

	public:
		Type_factory();
		Type_factory(const Type_factory&) = delete;

		void register_type(size_t type, const Type_product& tp);
		Type_product operator[](size_t type);
		Type_product operator[](const boost::filesystem::path& p);

		const char* get_mime(const boost::filesystem::path& p);
		size_t get_hash_type(const boost::filesystem::path& p);

	private:
		static inline bool find_predicate(const Type_map::value_type& v, size_t find)
		{
			constexpr int half_size_t = sizeof(size_t) * 4;
			return (v.first >> half_size_t) == ((v.first >> half_size_t) & (find >> half_size_t));
		}
	};
}

#endif // HAWK_TYPE_FACTORY_H
