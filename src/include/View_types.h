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

#ifndef HAWK_VIEW_TYPES_H
#define HAWK_VIEW_TYPES_H

#include <unordered_map>
#include <functional>
#include <vector>
#include <utility>
#include <mutex>
#include "Path.h"
#include "View.h"

namespace hawk {
	class View_group;
	struct Magic_guard;

	class View_types{
	public:
		using Handler = std::function<View*(View_group&)>;

	private:
		mutable std::mutex m_mtx;

		Magic_guard* m_magic_guard;
		std::unordered_map<size_t, Handler> m_types;

	public:
		View_types();
		~View_types();

		View_types(const View_types&) = delete;

		// (not thread-safe)
		void register_type(size_t type, const Handler& tp);

		// Returns a handler for supplied hash or deduced file type.
		Handler get_handler(size_t type) const;
		Handler get_handler(const Path& p) const;

		std::string get_mime(const Path& p) const;
		size_t get_hash_type(const Path& p) const;
	};
}

#endif // HAWK_VIEW_TYPES_H
