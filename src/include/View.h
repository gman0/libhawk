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

#ifndef HAWK_VIEW_H
#define HAWK_VIEW_H

#include <memory>
#include <utility>
#include <vector>
#include "Path.h"

namespace hawk {
	class View_group;

	class View
	{
	public:
		class Ready_guard
		{
		private:
			View& v;

		public:
			Ready_guard(View& view) : v{view} { v.not_ready();}
			~Ready_guard() { v.ready(); }
		};

	protected:
		View_group* m_parent;
		Path m_path;

	public:
		View(View_group& parent) : m_parent{&parent} {}

		View(const View&) = delete;
		View& operator=(const View&) = delete;

		virtual ~View() = default;

		const Path& get_path() const;
		virtual void set_path(const Path& path);

		// These two methods are called internally by hawk::View_group,
		// encompassing set_path call. Views are thread-unsafe between
		// not_ready() and ready() calls. Do your synchronization in here.
		virtual void ready() noexcept = 0;
		virtual void not_ready() noexcept = 0;
	};
}

#endif // HAWK_VIEW_H
