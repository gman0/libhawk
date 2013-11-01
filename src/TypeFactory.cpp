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

#include "TypeFactory.h"
#include "Handler.h"

using namespace hawk;

void Type_factory::register_type(size_t type,
	const Type_factory::Type_product& tp)
{
	m_types[type] = tp;
}

Type_factory::Type_product Type_factory::operator[](size_t type)
{
	auto tp_iterator = m_types.find(type);

	if (tp_iterator == m_types.end())
		return Type_product{nullptr};

	return tp_iterator->second;
}
