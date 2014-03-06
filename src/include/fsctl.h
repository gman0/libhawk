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

}

#endif // FS_CTL_H
