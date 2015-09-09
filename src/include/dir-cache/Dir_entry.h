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

#ifndef HAWK_DIR_ENTRY_H
#define HAWK_DIR_ENTRY_H

#include <memory>
#include <vector>
#include <functional>
#include "Path.h"
#include "User_data.h"

namespace hawk {
	struct Dir_entry
	{
		Path path;
		User_data user_data;
	};

	using Dir_vector = std::vector<Dir_entry>;
	using Dir_cursor = Dir_vector::iterator;
	using Dir_const_cursor = Dir_vector::const_iterator;
	using Dir_ptr = std::shared_ptr<Dir_vector>;

	using Dir_sort_predicate = std::function<
		bool(const Dir_entry&, const Dir_entry&)>;
	using Populate_user_data = std::function<void(const Path&, User_data&)>;

	void set_populate_user_data(Populate_user_data&& populate_udata);
	void set_sort_predicate(Dir_sort_predicate&& pred);
	Dir_sort_predicate get_sort_predicate();

	void sort(Dir_vector& vec);
	void populate_user_data(const Path& parent, Dir_entry& ent);
	// Populates the directory with user data and sorts it if possible.
	void populate_directory(Dir_vector& vec, const Path& dir);
}

#endif // HAWK_DIR_ENTRY_H
