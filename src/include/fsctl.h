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

#ifndef FS_CTL_H
#define FS_CTL_H

#include <boost/filesystem/path.hpp>

namespace hawk {

class Tab;

void rename(const boost::filesystem::path& old_p,
	const boost::filesystem::path& new_p, Tab* tab);
void rename(const boost::filesystem::path& old_p,
	const boost::filesystem::path& new_p, Tab* tab,
	boost::system::error_code& ec) noexcept;

// Note that it's front-end's responsibility dest move the cursor after
// removing a file/directory.
// XXX: WORK IN PROGRESS!
bool remove(const boost::filesystem::path& p);
bool remove(const boost::filesystem::path&,
	boost::system::error_code& ec) noexcept;
// XXX: WORK IN PROGRESS!
uintmax_t remove_all(const boost::filesystem::path& p);
uintmax_t remove_all(const boost::filesystem::path& p,
	boost::system::error_code& ec) noexcept;

// XXX: WORK IN PROGRESS!
void copy(const boost::filesystem::path& src,
	const boost::filesystem::path& dest);
void copy(const boost::filesystem::path& src,
	const boost::filesystem::path& dest, boost::system::error_code& ec) noexcept;

// XXX: WORK IN PROGRESS!
void move(const boost::filesystem::path& src,
	const boost::filesystem::path& dest);
void move(const boost::filesystem::path& src,
	const boost::filesystem::path& dest,
	boost::system::error_code& ec) noexcept;

void create_directory(const boost::filesystem::path& p);
void create_directory(const boost::filesystem::path& p,
	boost::system::error_code& ec) noexcept;
void create_directories(const boost::filesystem::path& p);
void create_directories(const boost::filesystem::path& p,
	boost::system::error_code& ec) noexcept;

void create_symlink(const boost::filesystem::path& dest,
	const boost::filesystem::path& new_symlink);
void create_symlink(const boost::filesystem::path& dest,
	const boost::filesystem::path& new_symlink,
	boost::system::error_code& ec) noexcept;

void create_hard_link(const boost::filesystem::path& dest,
	const boost::filesystem::path& new_hard_link);
void create_hard_link(const boost::filesystem::path& dest,
	const boost::filesystem::path& new_hard_link,
	boost::system::error_code& ec) noexcept;

} // namespace hawk

#endif // FS_CTL_H
