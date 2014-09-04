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

#ifndef HAWK_DIR_CACHE_H
#define HAWK_DIR_CACHE_H

#include <memory>
#include <boost/filesystem/path.hpp>
#include "No_hash.h"

namespace hawk
{
	struct Dir_entry
	{
		std::time_t timestamp;
		boost::filesystem::path path;
		boost::filesystem::file_status status;
	};

	using Dir_vector = std::vector<Dir_entry>;
	using Dir_cursor = Dir_vector::iterator;
	using Dir_const_cursor = Dir_vector::const_iterator;

	class Dir_ptr
	{
	private:
		using Ptr = std::shared_ptr<Dir_vector>;
		Ptr m_ptr;

	public:
		Dir_ptr() {}
		Dir_ptr(Ptr& ptr);

		~Dir_ptr();

		Dir_ptr(Dir_ptr&& ptr) noexcept
			: m_ptr{std::move(ptr.m_ptr)} {}
		Dir_ptr& operator=(Dir_ptr&& ptr) noexcept;

		Dir_vector* get() const { return m_ptr.get(); }
		Dir_vector* operator->() const { return m_ptr.get(); }
		Dir_vector& operator*() const { return *m_ptr; }
	};

	Dir_ptr get_dir_ptr(const boost::filesystem::path& p, size_t path_hash);
}

#endif // HAWK_DIR_CACHE_H
