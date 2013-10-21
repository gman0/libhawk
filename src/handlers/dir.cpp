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

#include "handlers/dir.h"

using namespace hawk;

List_dir::List_dir(const boost::filesystem::path& path, const std::string& type)
	:
	Handler{path, type},
	m_cache{new Dir_cache}
{}

List_dir::~List_dir()
{
	delete m_cache;
}

Dir_cache::Dir_vector* Dir_cache::add_dir_entry(const size_t& hash)
{
	return (m_cache_dictionary[hash] = new Dir_vector);
}
