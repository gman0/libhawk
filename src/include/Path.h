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

#ifndef HAWK_PATH_H
#define HAWK_PATH_H

#include <string>
#include <utility>
#include <cstddef>
#include <type_traits>

namespace hawk {
	class Path
	{
	private:
		std::string m_path;
		mutable size_t m_hash;

		// Modify type deduction when dealing with perfect forwarding.
		template <typename T>
		using enable_if_not_self =
			typename std::enable_if<!std::is_same<
				typename std::decay<T>::type, Path>::value>::type;

	public:
		Path() : m_hash{0} {}

		template <typename Path_, enable_if_not_self<Path_>* = nullptr>
		Path(Path_&& p, size_t hash = 0)
			: m_path{std::forward<Path_>(p)}, m_hash{hash}
		{}

		template <typename Path_, enable_if_not_self<Path_>* = nullptr>
		Path& operator=(Path_&& p)
		{
			m_path = std::forward<Path_>(p);
			m_hash = 0;

			return *this;
		}

		Path(const char* p, std::string::size_type count)
			: m_path{p, (count + 1)}, m_hash{0}
		{}

		void clear();
		bool empty() const;

		// Compare two paths by comparing their hashes.
		bool operator==(const Path& p);

		// Concatenate two paths.
		Path& operator/=(const Path& p);

		const char* c_str() const;
		const std::string& string() const;
		std::wstring wstring() const;

		int compare(const Path& other) const;
		int compare(const std::wstring& other) const;

		Path parent_path() const;
		void set_parent_path();

		Path filename() const;
		void set_filename();

		bool is_absolute() const;

		// Even though this method is marked as const it
		// can change the value of m_hash.
		size_t hash() const;

	private:
		void append_separator_if_needed();
	};

	// Concatenate two paths.
	Path operator/(const Path& lhs, const Path& rhs);
}

#endif // HAWK_PATH_H
