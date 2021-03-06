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

#ifndef HAWK_NOT_SELF_TRAIT_H
#define HAWK_NOT_SELF_TRAIT_H

#include <type_traits>

namespace hawk {
	template <typename T, typename Self>
	using enable_if_not_self =
		std::enable_if_t<!std::is_same<
			std::decay_t<T>, Self>::value>;
}

#endif // HAWK_NOT_SELF_TRAIT_H
