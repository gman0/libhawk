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

#ifndef HAWK_LIST_DIR_HASH_H
#define HAWK_LIST_DIR_HASH_H

#include <functional>
#include "handlers/dir.h"
#include "handlers/hash.h"
#include "calchash.h"

static std::string type { "inode/directory" };
static size_t hash = hawk::calculate_mime_hash(type);

namespace hawk {
	template <>
	size_t get_handler_hash<List_dir>()
	{
		return hash;
	}
}

#endif // HAWK_LIST_DIR_HASH_H
