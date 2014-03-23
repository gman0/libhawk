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

using namespace boost::filesystem;

namespace hawk {

void rename(const path& old_p, const path& new_p, Tab* tab)
{
	boost::filesystem::rename(old_p, new_p);

	// Update the cursor if the file/directory we're renaming
	// is focused by a cursor.

	path parent = old_p.parent_path();
	size_t parent_hash = hash_value(parent);

	List_dir::Cursor_map::iterator it;
	if (tab->find_cursor(parent_hash, it))
	{
		if (it->second == hash_value(old_p))
			tab->store_cursor(parent_hash, hash_value(new_p));
	}
}

void rename(const path& old_p, const path& new_p, Tab* tab,
	boost::system::error_code& ec) noexcept
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

} // namespace hawk
