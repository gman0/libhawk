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
		Path m_path;

		// Views form a linked-list-like structure. This pointer
		// is used to set cursors properly.
		const View* m_next_view;

	public:
		View() : m_next_view{nullptr} {}
		virtual ~View() = default;

		View(const View&) = delete;
		View& operator=(const View&) = delete;

		View(View&& v) noexcept;
		View& operator=(View&& v) noexcept;

		// For internal/expert use only.
		void _set_next_view(const View* next_view);

		const Path& get_path() const;
		virtual void set_path(const Path& path);

		// These two methods are called internally by hawk::View_group,
		// encompassing set_path call. Views are thread-unsafe until ready()
		// is called. Do your synchronization in here.
		virtual void ready() noexcept = 0;
		virtual void not_ready() noexcept = 0;

	protected:
		// Returns nullptr when m_next_view is nullptr.
		// Otherwise pointer to path of the next view
		// is returned.
		const Path* get_next_path() const;
	};
}

#endif // HAWK_VIEW_H
