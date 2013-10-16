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

#ifndef HAWK_TYPE_FACTORY_H
#define HAWK_TYPE_FACTORY_H

#include <unordered_map>

namespace hawk {
	class Handler;
	using Type_product = Handler*(*)();

	class Type_factory
	{
	private:
		std::unordered_map<size_t, Type_product> m_types;

	public:
		void register_type(size_t type, Type_product tp);
		Type_product operator[](size_t type);
	};
}

#endif // HAWK_TYPE_FACTORY_H
