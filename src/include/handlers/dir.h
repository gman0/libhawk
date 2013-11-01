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
	class List_dir : public Handler
	{
	public:
		/*
		struct Dir_entry
		{
			std::time_t timestamp;
			boost::filesystem::path path;
			boost::filesystem::file_status status;
		};
		*/

		using Dir_entry = boost::filesystem::path;

		using Dir_vector = std::vector<Dir_entry>;
		using Dir_cursor = Dir_vector::iterator;

		struct Dir_cache
		{
			Dir_vector vec;
			Dir_cursor cursor;
		};

	private:
		Cache<size_t, Dir_cache> m_cache;
		Dir_cursor m_cursor;
		Dir_cache* m_active_cache;

	public:
		List_dir(const boost::filesystem::path& path, const std::string& type);
		const Dir_cache* read() { return m_active_cache; }

		// TODO: navigate, move_cursor etc...

	private:
		void fill_cache(Dir_cache* dc);
	};
}

#endif // HAWK_LIST_DIR_H