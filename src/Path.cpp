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

#include <functional>
#include <cstdlib>
#include <cassert>
#include <clocale>
#include <cwchar>
#include "Path.h"

namespace hawk {

void Path::clear()
{
	m_path.clear();
	m_hash = 0;
}

bool Path::empty() const
{
	return m_path.empty();
}

bool operator==(const Path& rhs, const Path& lhs)
{
	return const_cast<Path&>(rhs).hash() == const_cast<Path&>(lhs).hash();
}

bool operator!=(const Path& rhs, const Path& lhs)
{
	return const_cast<Path&>(rhs).hash() != const_cast<Path&>(lhs).hash();
}

Path& Path::operator/=(const Path& p)
{
	if (p.empty())
		return *this;

	if (this != &p)
	{
		if (p.m_path.front() != '/')
			append_separator_if_needed();

		m_path += p.m_path;
	}
	else
	{
		Path rhs {p};
		if (rhs.m_path.front() != '/')
			append_separator_if_needed();

		m_path += rhs.m_path;
	}

	m_hash = 0;

	return *this;
}

Path operator/(const Path& lhs, const Path& rhs)
{
	return Path(lhs) /= rhs;
}

const char* Path::c_str() const
{
	return m_path.c_str();
}

const std::string& Path::string() const
{
	return m_path;
}

std::wstring Path::wstring() const
{
	std::mbstate_t state {};
	const char* src = m_path.c_str();

	std::wstring wstr(m_path.length(), L' ');
	wstr.resize(mbsrtowcs(&wstr[0], &src, m_path.length(), &state));

	return wstr;
}

int Path::compare(const Path& other) const
{
	std::wstring lhs = wstring();
	std::wstring rhs = other.wstring();

	return lhs.compare(rhs);
}

int Path::compare(const std::wstring& other) const
{
	std::wstring lhs = wstring();
	return lhs.compare(other);
}

int Path::icompare(const Path& other) const
{
	return wcscasecmp(wstring().c_str(), other.wstring().c_str());
}

int Path::icompare(const std::wstring& other) const
{
	return wcscasecmp(wstring().c_str(), other.c_str());
}

Path Path::parent_path() const
{
	auto pos = m_path.rfind('/');

	if (pos == std::string::npos) return Path {};
	if (pos == 0)
	{
		return (m_path[1] == '\0') ? Path {} : Path {"/"};
	}

	return Path {m_path.c_str(), --pos};
}

void Path::set_parent_path()
{
	auto pos = m_path.rfind('/');

	if (pos == std::string::npos)
		m_path.clear();
	else
	{
		if (pos == 0 && m_path[1] != '\0')
			m_path = "/";
		else
			m_path = m_path.substr(0, pos - 1);
	}

	m_hash = 0;
}

Path Path::filename() const
{
	auto pos = m_path.rfind('/');

	if (pos == std::string::npos || pos == 1)
		return *this;

	return Path(m_path.c_str() + pos + 1,
				m_path.length() - pos - 2);
}

void Path::set_filename()
{
	auto pos = m_path.rfind('/');

	if (pos == std::string::npos || pos == 1)
		return;

	m_path = m_path.substr(pos - 1, m_path.length());
	m_hash = 0;
}

bool Path::is_absolute() const
{
	return !m_path.empty() && m_path[0] == '/';
}

size_t Path::hash() const
{
	if (m_hash == 0)
		m_hash = std::hash<std::string>()(m_path);

	return m_hash;
}

void Path::append_separator_if_needed()
{
	if (!m_path.empty() && m_path.back() != '/')
		m_path += '/';
}

} // namespace hawk
