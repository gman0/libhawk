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

#ifndef HAWK_LIST_DIR_H
#define HAWK_LIST_DIR_H

#include <vector>
#include <string>
#include <ctime>
#include <boost/filesystem.hpp>
#include "Handler.h"
#include "handlers/Cache.h"

namespace hawk {
	class Dir_cache;
	class List_dir : public Handler
	{
	public:
		struct Dir_entry
		{
			std::time_t timestamp;
			std::string name;
			boost::filesystem::file_status status;
		};

		using Dir_cursor = std::vector<Dir_entry>::iterator;

	private:
		Dir_cache* m_cache;
		Dir_cursor m_cursor;

	public:
		List_dir(const boost::filesystem::path& path, const std::string& type);
		~List_dir();

		// navigate
		// move_cursor

	// private:
		// ;
	};

	class Dir_cache : public Cache<size_t, std::vector<List_dir::Dir_entry>>
	{
	public:
		using Dir_vector = std::vector<List_dir::Dir_entry>;
		virtual Dir_vector* add_dir_entry(const size_t& hash);
	};
}

#endif // HAWK_LIST_DIR_H
