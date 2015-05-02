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
#include <climits>
#include <cstddef>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
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

void Directory_iterator::advance(const Directory_iterator& target)
{
	while (*this != target || !m_dir)
		operator++();
}

Path Directory_iterator::operator*() const
{
	return (m_dir) ? Path{m_dir->ent->d_name} : Path{};
}

bool Directory_iterator::operator==(const Directory_iterator& it) const
{
	if (!m_dir && !it.m_dir) return true;
	if (!m_dir ^ !static_cast<bool>(it.m_dir)) return false;

	return m_dir->ent->d_ino == it.m_dir->ent->d_ino;
}

bool Directory_iterator::operator!=(const Directory_iterator& it) const
{
	return !operator==(it);
}

bool Directory_iterator::at_end() const
{
	return !m_dir;
}

/// end of Directory_iterator implementation

Path Recursive_directory_iterator::operator*() const
{
	Path p;
	for (const auto& leaf : m_iter_stack)
		p /= *leaf.second;

	return p;
}

Path Recursive_directory_iterator::top() const
{
	return *m_iter_stack.back().second;
}

int Recursive_directory_iterator::level() const
{
	return m_iter_stack.size() - 1;
}

bool Recursive_directory_iterator::operator==(
		const Recursive_directory_iterator& it) const
{
	if (m_iter_stack.empty() && it.m_iter_stack.empty())
		return true;

	if (m_iter_stack.empty() ^ it.m_iter_stack.empty())
		return false;

	return m_iter_stack.back().second == it.m_iter_stack.back().second;
}

bool Recursive_directory_iterator::operator!=(
		const Recursive_directory_iterator& it) const
{
	return !operator==(it);
}

bool Recursive_directory_iterator::at_end() const
{
	return m_iter_stack.empty();
}

Recursive_directory_iterator::Recursive_directory_iterator(const Path& p)
{
	m_iter_stack.emplace_back(p, Directory_iterator {p});
}

void Recursive_directory_iterator::leave_directory()
{
	if (!m_iter_stack.empty())
		m_iter_stack.pop_back();
}

Recursive_directory_iterator& Recursive_directory_iterator::operator++()
{
	if (m_iter_stack.empty()) return *this;

	auto& back = m_iter_stack.back();
	Path parent_path =  back.first / *back.second;

	if (is_directory(parent_path))
		m_iter_stack.emplace_back(parent_path, Directory_iterator {parent_path});
	else
		increment();

	resolve_empty_directories();

	return *this;
}

Recursive_directory_iterator&
Recursive_directory_iterator::orthogonal_increment()
{
	if (!m_iter_stack.empty())
	{
		increment();
		resolve_empty_directories();
	}

	return *this;
}

void Recursive_directory_iterator::advance(
		const Recursive_directory_iterator& target)
{
	while (operator!=(target) || m_iter_stack.back().second.at_end())
		operator++();
}

void Recursive_directory_iterator::resolve_empty_directories()
{
	while (m_iter_stack.back().second.at_end())
	{
		m_iter_stack.pop_back();
		if (m_iter_stack.empty()) break;
		increment();
	}
}

/// end of Recursive_directory_iterator implementation

// query functions

bool exists(const Path& p)
{
	return access(p.c_str(), F_OK) == 0;
}

Stat status(const Path& p)
{
	Stat st;
	if (lstat64(p.c_str(), &st) == -1)
		throw Filesystem_error {p, errno};

	return st;
}

Stat status(const Path& p, int& err) noexcept
{
	Stat st;
	if (lstat64(p.c_str(), &st) == -1)
		err = errno;
	else
		err = 0;

	return st;
}

bool is_readable(const Path& p) noexcept
{
	return access(p.c_str(), R_OK) == 0;
}

bool is_readable(const Stat& st) noexcept
{
	return st.st_mode & S_IREAD;
}

bool is_writable(const Path& p) noexcept
{
	return access(p.c_str(), W_OK) == 0;
}

bool is_writable(const Stat& st) noexcept
{
	return st.st_mode & S_IWRITE;
}

bool is_directory(const Path& p)
{
	return S_ISDIR(status(p).st_mode);
}

bool is_directory(const Stat& st) noexcept
{
	return S_ISDIR(st.st_mode);
}

bool is_regular_file(const Path& p)
{
	return S_ISREG(status(p).st_mode);
}

bool is_regular_file(const Stat& st) noexcept
{
	return S_ISREG(st.st_mode);
}

bool is_symlink(const Path& p)
{
	return S_ISLNK(status(p).st_mode);
}

bool is_symlink(const Stat& st) noexcept
{
	return S_ISLNK(st.st_mode);
}

uintmax_t file_size(const Path& p)
{
	return status(p).st_size;
}

uintmax_t file_size(const Stat& st) noexcept
{
	return st.st_size;
}

time_t last_write_time(const Path& p)
{
	return status(p).st_ctim.tv_sec;
}

time_t last_write_time(const Path& p, int& err) noexcept
{
	Stat st;
	if (stat64(p.c_str(), &st) == -1)
	{
		err = errno;
		return -1;
	}

	err = 0;

	return st.st_mtim.tv_sec;
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

	return Path {buf.ptr};
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

	err = 0;

	return Path {buf.ptr};
}

Space_info space(const Path& p)
{
	struct statvfs64 stvfs;
	if (statvfs64(p.c_str(), &stvfs) != 0)
		throw Filesystem_error {p, errno};

	return { stvfs.f_blocks * stvfs.f_frsize,
			 stvfs.f_bfree  * stvfs.f_frsize,
			 stvfs.f_bavail * stvfs.f_frsize };
}

// operational functions

void create_directory(const Path& p)
{
	if (mkdir(p.c_str(), 0777) == -1)
		throw Filesystem_error {p, errno};
}

void create_directory(const Path& p, int& err) noexcept
{
	if (mkdir(p.c_str(), 0777) == -1)
	{
		err = errno;
		return;
	}

	err = 0;
}

void create_directories(const Path& p)
{
	char str[PATH_MAX];
	char* s = str + 1; // +1: Assuming p is an asbolute path.
	strcpy(str, p.c_str());

	while ((s = strchr(str, '/')))
	{
		*s = '\0';

		if (mkdir(str, 0777) == -1 && errno != EEXIST)
			throw Filesystem_error {str, errno};

		*s = '/';
		s++;
	}
}

void create_directories(const Path& p, int& err) noexcept
{
	char str[PATH_MAX];
	char* s = str + 1; // +1: Assuming p is an asbolute path.
	strcpy(str, p.c_str());

	while ((s = strchr(str, '/')))
	{
		*s = '\0';

		if (mkdir(str, 0777) == -1 && errno != EEXIST)
		{
			err = errno;
			return;
		}


		*s = '/';
		s++;
	}

	err = 0;
}

void create_symlink(const Path& target, const Path& linkpath)
{
	if (symlink(target.c_str(), linkpath.c_str()) != 0)
		throw Filesystem_error {linkpath, errno};
}

void create_symlink(const Path& target,
					const Path& linkpath, int& err) noexcept
{
	if (symlink(target.c_str(), linkpath.c_str()) != 0)
		err = errno;
	else
		err = 0;
}

int read_permissions(const Path& p)
{
	return status(p).st_mode;
}

int read_permissions(const Stat& st) noexcept
{
	return st.st_mode;
}

void write_permissions(const Path& p, mode_t perms)
{
	if (chmod(p.c_str(), perms) != 0)
		throw Filesystem_error {p, errno};
}

void write_permissions(const Path& p, mode_t perms, int& err) noexcept
{
	if (chmod(p.c_str(), perms) != 0)
		err = errno;
	else
		err = 0;
}

void copy_permissions(const Path& from, const Path& to)
{
	if (chmod(to.c_str(), status(from).st_mode) != 0)
		throw Filesystem_error {to, errno};
}

} // namespace hawk
