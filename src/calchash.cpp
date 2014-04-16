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

#include <cstring>
#include "calchash.h"

namespace hawk {

size_t calculate_mime_hash(std::string mime)
{
	// A little explanation might be needed:
	// A mime-type string consists of a category
	// and its specific type (ie. CATEGORY/SPECIFIC_TYPE).

	constexpr int half_size_t = sizeof(size_t) * 4;
	static char split[256];
	
	// First, we need to check whether we even have
	// the second (specific) part of the mime-type.
	auto slash_pos = mime.find('/');
	if (slash_pos == std::string::npos)
	{
		// (we don't)
		// The lower half is zeroed out which means
		// we won't check for the specific mime-type.
		return (std::hash<std::string>()(mime) << half_size_t);
	}

	strcpy(split, mime.c_str());

	// End the string exactly where the '/' was so we can
	// calculate the category's hash.
	split[slash_pos] = '\0';
	mime = split;

	// Shift it to the upper half (leaving only the lower half
	// of the first hash itself).
	size_t hash = std::hash<std::string>()(mime) << half_size_t;

	// Now we'll calculate the 2nd hash which is located just
	// past the '/' character (which is now '\0').
	mime = split + slash_pos + 1;
	// The calculated hash has will now fill the lower half of
	// the resulting hash.
	hash |= std::hash<std::string>()(mime) >> half_size_t;

	return hash;
}

} // namespace hawk
