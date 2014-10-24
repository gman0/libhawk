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

#include "User_data.h"

namespace hawk {

User_data::~User_data()
{
	delete m_content;
}

User_data::User_data(const User_data& ud)
	: m_content(ud.m_content ? ud.m_content->clone() : nullptr)
{}

User_data::User_data(User_data&& ud) noexcept
	: m_content{ud.m_content}
{
	ud.m_content = nullptr;
}

User_data& User_data::swap(User_data& ud) noexcept
{
	std::swap(m_content, ud.m_content);
	return *this;
}

User_data& User_data::operator=(const User_data& ud)
{
	User_data{ud}.swap(*this);
	return *this;
}

User_data& User_data::operator=(User_data&& rhs) noexcept
{
	User_data{rhs}.swap(*this);
	return *this;
}

bool User_data::empty() const
{
	return !m_content;
}

void User_data::clear()
{
	User_data{}.swap(*this);
}

const std::type_info& User_data::type() const
{
	return m_content ? m_content->type() : typeid(void);
}

} // namespace hawk
