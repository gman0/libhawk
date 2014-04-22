/*
	Copyright (C) 2013-2014 Róbert "gman" Vašek <gman@codefreax.org>

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

#include <boost/functional/hash.hpp>
#include "Tab.h"
#include "fsctl.h"
#include "handlers/List_dir.h"

using boost::filesystem::path;
using boost::filesystem::hash_value;
using boost::system::error_code;
using boost::filesystem::filesystem_error;

namespace hawk {

static void copy_preserve_name(const path& src, const path& dest)
{
	if (exists(dest))
		boost::filesystem::copy(src, dest / src.filename());
	else
		boost::filesystem::copy(src, dest);
}

static void copy_dir_preserve_name(const path& src, path& dest)
{
	if (exists(dest))
	{
		dest /= src.filename();
		boost::filesystem::copy_directory(src, dest);
	}
	else
		boost::filesystem::copy_directory(src, dest);
}

void rename(const path& old_p, const path& new_p, Tab* tab)
{
	boost::filesystem::rename(old_p, new_p);

	// Update the cursor if the file/directory we're renaming
	// is focused by a cursor.

	path parent = old_p.parent_path();
	size_t parent_hash = hash_value(parent);

	List_dir::Cursor_map::iterator it;
	if (tab->find_cursor(parent_hash, it)
		&& it->second == hash_value(old_p))
	{
		tab->store_cursor(parent_hash, hash_value(new_p));
	}
}

void rename(const path& old_p, const path& new_p, Tab* tab,
	error_code& ec) noexcept
{
	try
	{
		rename(old_p, new_p, tab);
	}
	catch (const filesystem_error& e)
	{
		ec = e.code();
	}
}

// XXX: WORK IN PROGRESS!
bool remove(const path& p)
{ return boost::filesystem::remove(p); }
bool remove(const path& p, error_code& ec) noexcept
{ return boost::filesystem::remove(p, ec); }

// XXX: WORK IN PROGRESS!
uintmax_t remove_all(const path& p)
{ return boost::filesystem::remove_all(p); }
uintmax_t remove_all(const path& p, error_code& ec) noexcept
{ return boost::filesystem::remove_all(p, ec); }

// XXX: WORK IN PROGRESS!
void copy(const path& src, const path& dst)
{
	if (is_regular_file(src) || is_symlink(src))
		copy_preserve_name(src, dst);
	else if (is_directory(src))
	{
		path dest {dst};
		copy_dir_preserve_name(src, dest);

		size_t src_len = src.native().length();
		for (boost::filesystem::recursive_directory_iterator end,
				it {src}; it != end; ++it)
		{
			const path& p = *it;
			copy_preserve_name(*it, dest / (p.native().c_str() + src_len));
		}
	}
}

void copy(const path& src, const path& dest, error_code& ec) noexcept
{
	try
	{
		hawk::copy(src, dest);
	}
	catch (const filesystem_error& e)
	{
		ec = e.code();
	}
}

// XXX: WORK IN PROGRESS!
void move(const path& src, const path& dst)
{
	if (is_regular_file(src) || is_symlink(src))
		copy_preserve_name(src, dst);
	else if (is_directory(src))
	{
		path dest { dst };
		copy_dir_preserve_name(src, dest);

		size_t src_len = src.native().length();
		for (boost::filesystem::recursive_directory_iterator end,
				it {src}; it != end; ++it)
		{
			const path& p = *it;
			copy_preserve_name(*it, dest / (p.native().c_str() + src_len));
			boost::filesystem::remove_all(*it);
		}
	}

	boost::filesystem::remove(src);
}

void move(const path& src, const path& dest, error_code& ec) noexcept
{
	try
	{
		hawk::move(src, dest);
	}
	catch (const filesystem_error& e)
	{
		ec = e.code();
	}
}

void create_directory(const path& p)
{ boost::filesystem::create_directory(p); }
void create_directory(const path& p, error_code& ec) noexcept
{ boost::filesystem::create_directory(p, ec); }

void create_directories(const path& p)
{ boost::filesystem::create_directories(p); }
void create_directories(const path& p, error_code& ec) noexcept
{ boost::filesystem::create_directories(p, ec); }

void create_symlink(const path& src, const path& new_symlink)
{ boost::filesystem::create_symlink(src, new_symlink); }
void create_symlink(const path& src, const path& new_symlink,
	error_code& ec) noexcept
{ boost::filesystem::create_symlink(src, new_symlink, ec); }

void create_hard_link(const path& src, const path& new_hard_link)
{ boost::filesystem::create_hard_link(src, new_hard_link); }
void create_hard_link(const path& src, const path& new_hard_link,
	error_code& ec) noexcept
{ boost::filesystem::create_hard_link(src, new_hard_link, ec); }

} // namespace hawk
