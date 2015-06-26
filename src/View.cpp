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

#include <utility>
#include <exception>
#include "View.h"
#include "View_group.h"

namespace hawk {

View::View(View&& v) noexcept
	:
	  m_path{std::move(v.m_path)},
	  m_next_view{v.m_next_view}
{
	v.m_next_view = nullptr;
}

View& View::operator=(View&& v) noexcept
{
	if (&v == this)
		return *this;

	m_path = std::move(v.m_path);
	m_next_view = v.m_next_view;

	v.m_next_view = nullptr;

	return *this;
}

void View::_set_next_view(const View* next_view)
{
	m_next_view = next_view;
}

const Path& View::get_path() const
{
	return m_path;
}

const Path* View::get_next_path() const
{
	return m_next_view ? &m_next_view->get_path() : nullptr;
}

void View::set_path(const Path& p)
{
	m_path = p;
}

} // namespace hawk
