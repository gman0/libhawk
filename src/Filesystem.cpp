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

#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <climits>
#include <cstddef>
#include <unistd.h>

#include "Filesystem.h"

namespace hawk {

size_t dirent_buf_size()
{
	long name_max;
	size_t name_end;

#   if defined(HAVE_FPATHCONF) && defined(HAVE_DIRFD) \
	   && defined(_PC_NAME_MAX)
		name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);
		if (name_max == -1)
#           if defined(NAME_MAX)
				name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#           else
				return static_cast<size_t>(-1);
#           endif
#   else
#       if defined(NAME_MAX)
			name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#       else
#           error "buffer size for readdir_r cannot be determined"
#       endif
#   endif
	name_end = static_cast<size_t>(offsetof(dirent, d_name)) + name_max + 1;
	return (name_end > sizeof(struct dirent)
			? name_end : sizeof(struct dirent));
}

Directory_iterator::Directory_iterator(const Path& p)
{
	DIR* d = opendir(p.c_str());

	if (d == nullptr)
		throw Filesystem_error {p, errno};

	m_dir = std::make_shared<Dir_guard>();
	m_dir->d = d;
	m_dir->ent = reinterpret_cast<dirent*>(new char[dirent_buf_size()]);

	operator++();
}

Directory_iterator& Directory_iterator::operator++()
{
	if (m_dir)
	{
		dirent* ent;
		if (readdir_r(m_dir->d, m_dir->ent, &ent) != 0)
			throw Filesystem_error {errno};

		if (ent == nullptr)
		{
			m_dir.reset();
			return *this;
		}

		if (strcmp(ent->d_name, ".") == 0
			|| strcmp(ent->d_name, "..") == 0)
			operator++();
	}

	return *this;
}

Path Directory_iterator::operator*()
{
	return (m_dir) ? Path{m_dir->ent->d_name} : Path{};
}

bool Directory_iterator::operator==(const Directory_iterator& it)
{
	return m_dir == it.m_dir;
}

bool Directory_iterator::operator!=(const Directory_iterator& it)
{
	return m_dir != it.m_dir;
}

// --- end of Directory_iterator implementation

bool exists(const Path& p)
{
	return access(p.c_str(), F_OK) == 0;
}

struct stat status(const Path& p)
{
	struct stat st;
	if (stat(p.c_str(), &st) == -1)
		throw Filesystem_error {p, errno};

	return st;
}

struct stat status(const Path& p, int& err) noexcept
{
	struct stat st;
	if (stat(p.c_str(), &st) == -1)
		err = errno;

	return st;
}

bool is_readable(const Path& p) noexcept
{
	return access(p.c_str(), R_OK) == 0;
}

bool is_readable(const struct stat& st) noexcept
{
	return st.st_mode & S_IREAD;
}

bool is_directory(const Path& p)
{
	return S_ISDIR(status(p).st_mode);
}

bool is_directory(const struct stat& st) noexcept
{
	return S_ISDIR(st.st_mode);
}

time_t last_write_time(const Path& p)
{
	return status(p).st_ctim.tv_sec;
}

time_t last_write_time(const Path& p, int& err) noexcept
{
	struct stat st;
	if (stat(p.c_str(), &st) == -1)
	{
		err = errno;
		return -1;
	}

	err = 0;

	return st.st_ctim.tv_sec;
}

Path canonical(const Path& p, const Path& base)
{
	thread_local static struct Buffer
	{
		char* ptr;
		Buffer() : ptr{new char[PATH_MAX]} {}
		~Buffer() { delete [] ptr; }
	} buf;

	char* res = (p.is_absolute()) ? realpath(p.c_str(), buf.ptr)
								  : realpath((base / p).c_str(), buf.ptr);

	if (res == nullptr)
		throw Filesystem_error {{base / p}, errno};

	return buf.ptr;
}

Path canonical(const Path& p, const Path& base, int& err) noexcept
{
	thread_local static struct Buffer
	{
		char* ptr;
		Buffer() : ptr{new char[PATH_MAX]} {}
		~Buffer() { delete [] ptr; }
	} buf;

	if (realpath((base / p).c_str(), buf.ptr) == nullptr)
	{
		err = errno;
		return Path {};
	}

	return Path {buf.ptr};
}

} // namespace hawk
