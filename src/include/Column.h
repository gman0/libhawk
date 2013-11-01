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

#ifndef HAWK_COLUMN_H
#define HAWK_COLUMN_H

#include <memory>
#include <utility>
#include "Handler.h"
#include "handlers/dir.h"
#include "TypeFactory.h"

namespace hawk {
	class Column
	{
	private:
		boost::filesystem::path m_path;
		std::shared_ptr<Handler> m_handler;

	public:
		Column(const boost::filesystem::path& path,
			const Type_factory::Type_product& tp);
		Column(boost::filesystem::path&& path,
			const Type_factory::Type_product& tp);

		Column(const Column& col)
			:
			m_path{col.m_path},
			m_handler{col.m_handler}
		{}

		Column(Column&& col)
			:
			m_path{std::move(col.m_path)},
			m_handler{std::move(col.m_handler)}
		{}

		Column& operator=(const Column& col);
		Column& operator=(Column&& col);
	};
}

#endif // HAWK_COLUMN_H
