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

#ifndef HAWK_HANDLER_H
#define HAWK_HANDLER_H

#include <boost/filesystem/path.hpp>
#include <string>
#include <utility>

namespace hawk {
	class Column;

	class Handler
	{
	protected:
		const boost::filesystem::path* m_path;
		size_t m_type; // hash value of type

		const Column* m_parent_column;

	public:
		Handler()
			:
			m_path{},
			m_type{},
			m_parent_column{}
		{}

		// type as in the mime type
		Handler(const boost::filesystem::path& path,
			const Column* parent_column,
			const std::string& type);

		Handler(const boost::filesystem::path& path,
			const Column* parent_column,
			size_t hash)
			:
			m_path{&path},
			m_type{hash},
			m_parent_column{parent_column}
		{}

		Handler(const Handler& h)
			:
			m_path{h.m_path},
			m_type{h.m_type},
			m_parent_column{h.m_parent_column}
		{}

		Handler(Handler&& h)
			:
			m_path{h.m_path},
			m_type{h.m_type},
			m_parent_column{h.m_parent_column}
		{}

		virtual ~Handler() {}

		size_t get_type() const;
		bool operator==(const Handler& h);
		bool operator!=(const Handler& h);

		virtual void set_path(const boost::filesystem::path& p);

		const Column* get_parent_column() const;
	};
}

#endif // HAWK_HANDLER_H
