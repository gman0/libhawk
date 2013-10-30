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

#include <boost/filesystem.hpp>
#include "Tab.h"
#include "TypeFactory.h"
#include "handlers/dir.h"

using namespace hawk;
using namespace boost::filesystem;

Tab::Tab(const boost::filesystem::path& pwd, unsigned columns,
	Type_factory* tf)
	:
	m_pwd{pwd},
	m_type_factory{tf}
{
	Type_factory::Type_product type_closure =
		[&pwd]() -> Handler* { return new List_dir(pwd, "inode/directory"); };

	for (unsigned i = 0; i < columns; i++)
		m_columns.push_back({pwd, type_closure}); // TODO: supply proper paths
}

const path& Tab::get_pwd() const
{
	return m_pwd;
}

void Tab::set_pwd(const path& pwd)
{
	m_pwd = pwd;
}
