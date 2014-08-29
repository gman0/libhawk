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

#ifndef HAWK_COLUMN_H
#define HAWK_COLUMN_H

#include <memory>
#include <utility>
#include <vector>
#include <boost/filesystem/path.hpp>

namespace hawk {
	class Column
	{
	protected:
		boost::filesystem::path m_path;
		const Column* m_next_column;

	public:
		Column() : m_next_column{}
		{}

		Column(const boost::filesystem::path& path)
			:
			  m_path{path},
			  m_next_column{}
		{}

		Column(boost::filesystem::path&& path)
			:
			  m_path{std::move(path)},
			  m_next_column{}
		{}

		virtual ~Column() = default;

		// For internal/expert use only.
		void _set_next_column(const Column* next_column);

		const boost::filesystem::path& get_path() const;
		virtual void set_path(const boost::filesystem::path& path);

		// Returns nullptr when m_next_column is nullptr.
		// Otherwise pointer to path of the next column
		// is returned.
		const boost::filesystem::path* get_next_path() const;
	};
}

#endif // HAWK_COLUMN_H
