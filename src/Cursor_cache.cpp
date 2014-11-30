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

#include "Cursor_cache.h"

namespace hawk {

bool Cursor_cache::find(size_t cursor_hash, Cursor_cache::Cursor& iter)
{
	iter = m_cursor_map.find(cursor_hash);
	return (iter != m_cursor_map.end());
}

void Cursor_cache::store(size_t key, size_t cursor_hash)
{
	m_cursor_map[key] = cursor_hash;
}

} // namespace hawk
