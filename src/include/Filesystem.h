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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iterator>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include "Path.h"

namespace hawk {
	class Directory_iterator
	{
	public:
		using value_type = Path;
		using iterator_category = std::forward_iterator_tag;

	private:
		struct Dir_guard
		{
			DIR* d;
			dirent* ent;
			~Dir_guard() { closedir(d); delete [] ent; }
		};

		std::shared_ptr<Dir_guard> m_dir;

	public:
		Directory_iterator() {}
		explicit Directory_iterator(const Path& p);

		Path operator*();

		bool operator==(const Directory_iterator& it);
		bool operator!=(const Directory_iterator& it);

		Directory_iterator& operator++();
	};

	class Filesystem_error : public std::exception
	{
	private:
		std::string m_what;

	public:
		Filesystem_error(const Path& p, int err)
			: m_what{p.c_str()}
		{
			char msg[128];
			msg[127] = '\0';
			m_what += ": "; m_what += strerror_r(err, msg, 127);
		}

		Filesystem_error(int err)
		{
			char msg[128];
			msg[127] = '\0';
			m_what = strerror_r(err, msg, 127);
		}
	};

	bool exists(const Path& p);

	struct stat status(const Path& p);
	struct stat status(const Path& p, int& err) noexcept;

	bool is_readable(const Path& p) noexcept;
	bool is_readable(const struct stat& st) noexcept;

	bool is_directory(const Path& p);
	bool is_directory(const struct stat& st) noexcept;

	time_t last_write_time(const Path& p);
	time_t last_write_time(const Path& p, int& err) noexcept;

	Path canonical(const Path& p, const Path& base);
	Path canonical(const Path& p, const Path& base, int& err) noexcept;
}

#endif // FILESYSTEM_H
