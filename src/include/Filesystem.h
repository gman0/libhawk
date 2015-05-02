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

#ifndef HAWK_FILESYSTEM_H
#define HAWK_FILESYSTEM_H

#include <iterator>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <deque>
#include <dirent.h>
#include <sys/stat.h>
#include "Path.h"

namespace hawk {
	class Recursive_directory_iterator;
	class Directory_iterator
	{
	public:
		using value_type = Path;
		using iterator_category = std::forward_iterator_tag;
		using difference_type = int;
		using pointer = Path*;
		using reference = Path&;

	private:
		struct Dir_guard
		{
			DIR* d;
			dirent* ent;
			~Dir_guard() { closedir(d); delete [] ent; }
		};

		std::shared_ptr<Dir_guard> m_dir;

		friend class Recursive_directory_iterator;

	public:
		constexpr Directory_iterator() {}
		explicit Directory_iterator(const Path& p);

		Path operator*() const;

		bool operator==(const Directory_iterator& it) const;
		bool operator!=(const Directory_iterator& it) const;

		// Returns true if there are no more items to iterate.
		bool at_end() const;

		Directory_iterator& operator++();

		// Advances this iterator until it either reaches
		// target or becomes null.
		void advance(const Directory_iterator& target);
	};

	class Recursive_directory_iterator
	{
	public:
		using value_type = Directory_iterator::value_type;
		using iterator_category = Directory_iterator::iterator_category;
		using difference_type = Directory_iterator::difference_type;
		using pointer = Directory_iterator::pointer;
		using reference = Directory_iterator::reference;

	private:
		std::deque<std::pair<Path, Directory_iterator>> m_iter_stack;

	public:
		Recursive_directory_iterator() {}
		explicit Recursive_directory_iterator(const Path& p);

		// Returns path relative to the top directory.
		Path operator*() const;
		// Returns only the filename.
		Path top() const;

		bool operator==(const Recursive_directory_iterator& it) const;
		bool operator!=(const Recursive_directory_iterator& it) const;

		// Returns true if there are no more items to iterate.
		bool at_end() const;

		// Returns the level of recursion. Zero means we're
		// in the top directory.
		int level() const;

		void leave_directory();

		Recursive_directory_iterator& operator++();
		// Increment without entering a sub-directory.
		Recursive_directory_iterator& orthogonal_increment();

		// Advances this iterator until it either reaches
		// target or becomes null.
		void advance(const Recursive_directory_iterator& target);

	private:
		inline void increment() { ++m_iter_stack.back().second; }
		void resolve_empty_directories();
	};

	class Filesystem_error : public std::exception
	{
	private:
		std::string m_what;
		int m_errno;

	public:
		Filesystem_error(const Path& p, int err)
			: m_what{p.c_str()}, m_errno{err}
		{
			char msg[128];
			msg[127] = '\0';
			m_what += ": "; m_what += strerror_r(err, msg, 127);
		}

		Filesystem_error(int err) : m_errno{err}
		{
			char msg[128];
			msg[127] = '\0';
			m_what = strerror_r(err, msg, 127);
		}

		virtual const char* what() const noexcept
		{
			return m_what.c_str();
		}

		int get_errno() const { return m_errno; }
	};

	// query functions

	using Stat = struct stat64;

	struct Space_info
	{
		uintmax_t capacity;
		uintmax_t free;
		uintmax_t available;
	};

	bool exists(const Path& p);

	Stat status(const Path& p);
	Stat status(const Path& p, int& err) noexcept;

	bool is_readable(const Path& p) noexcept;
	bool is_readable(const Stat& st) noexcept;

	bool is_writable(const Path& p) noexcept;
	bool is_writable(const Stat& st) noexcept;

	bool is_directory(const Path& p);
	bool is_directory(const Stat& st) noexcept;

	bool is_regular_file(const Path& p);
	bool is_regular_file(const Stat& st) noexcept;

	bool is_symlink(const Path& p);
	bool is_symlink(const Stat& st) noexcept;

	uintmax_t file_size(const Path& p);
	uintmax_t file_size(const Stat& st) noexcept;

	time_t last_write_time(const Path& p);
	time_t last_write_time(const Path& p, int& err) noexcept;

	int read_permissions(const Path& p);
	int read_permissions(const Stat& st) noexcept;

	Path canonical(const Path& p, const Path& base);
	Path canonical(const Path& p, const Path& base, int& err) noexcept;

	Space_info space(const Path& p);
	Space_info space(const Path& p, int& err) noexcept;

	// operational functions

	void write_permissions(const Path& p, mode_t perms);
	void write_permissions(const Path& p, mode_t perms, int& err) noexcept;

	void copy_permissions(const Path& from, const Path& to);

	void create_directory(const Path& p);
	void create_directory(const Path& p, int& err) noexcept;
	void create_directories(const Path& p);
	void create_directories(const Path& p, int& err) noexcept;

	void create_symlink(const Path& target, const Path& linkpath);
	void create_symlink(const Path& target, const Path& linkpath,
						int& err) noexcept;
}

#endif // HAWK_FILESYSTEM_H
