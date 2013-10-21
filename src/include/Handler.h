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

namespace hawk {
	class Handler
	{
	private:
		// this can be a reference because Column always
		// outlives its Handler
		const boost::filesystem::path& m_path;
		size_t m_type; // hash value of type

	public:
		// type as in the mime type
		Handler(const boost::filesystem::path& path, const std::string& type);
		virtual const char* next() = 0; // get the next item in the item container (if any)

		size_t get_type() const;
		bool operator==(const Handler& h);
		bool operator!=(const Handler& h);
	};
}

#endif // HAWK_HANDLER_H
