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

#ifndef HAWK_TAB_H
#define HAWK_TAB_H

#include <vector>
#include <memory>
#include <utility>
#include <exception>
#include <shared_mutex>
#include <chrono>
#include "Path.h"
#include "View_types.h"
#include "dir-cache/Dir_entry.h"
#include "Tasking.h"
#include "handlers/List_dir.h"

namespace hawk {
	class Cursor_cache;
	class View;
	class List_dir;

	/// Exception safety
	//  User's code can safely throw exceptions with these guarantees:
	//   * throwing an exception while adding a view ceases its construction,
	//   * throwing an exception while creating a preview
	//     (ie. constructor, set_path, ready) causes the preview
	//     to cease its construction,
	//   * throwing an exception while setting View_group path by calling
	//     set_path/reload_path (which subsequently calls set_path of every
	//     View) causes the path to be reset to its previous value. If this
	//     call fails  again by throwing an exception, the parent path of
	//     current path will be used - this will continue in a loop until
	//     call to set_path succeeds (thus the path may end up being "/").
	//
	//  Any exception thrown in any of these scenarios will be rethrown back
	//  to the user via the exception handler.
	//
	/// Thread safety and threads
	//  All methods are thread-safe unless stated otherwise.
	//  set_path and reload_path are executed asynchronously, creating a preview
	//  is asynchronous as well.
	//  Subsequent calls to set_path/reload_path may result in an interrupt
	//  which causes any set-path task that may be running at the
	//  moment to stop--replacing it with the most recent set-path task.

	class View_group
	{
	public:
		using View_ptr = std::unique_ptr<View>;
		using List_dir_ptr = std::unique_ptr<List_dir>;
		using List_dir_vector = std::vector<List_dir_ptr>;
		using Exception_handler = std::function<
			void(std::exception_ptr) noexcept>;

	private:
		mutable std::shared_timed_mutex m_path_sm;
		Path m_path;

		mutable std::shared_timed_mutex m_preview_path_sm;
		Path m_preview_path;

		List_dir_vector m_views;
		View_ptr m_preview;
		std::chrono::milliseconds m_preview_delay;
		std::chrono::time_point<
			std::chrono::steady_clock> m_preview_timestamp;

		const View_types& m_view_types;
		Cursor_cache& m_cursor_cache;

		Tasking m_tasking;
		Tasking::Exception_handler m_exception_handler;

	public:
		View_group(
				const View_types& vt, Cursor_cache& cc,
				const Exception_handler& eh,
				std::chrono::milliseconds preview_delay)
			: m_preview_delay{preview_delay},
			  m_view_types{vt}, m_cursor_cache{cc},
			  m_tasking{eh}, m_exception_handler{eh}
		{}

		virtual ~View_group() = default;

		View_group(const View_group&) = delete;
		View_group& operator=(const View_group&) = delete;

		// Adds a List_dir view (or its derivates) to the view group.
		// Note that the View's set_path is NOT called. View_group's
		// set_path/reload_path needs to be called in order to get
		// View::set_path called.
		virtual void add_view(const View_types::Handler& list_dir);
		// Removes a single view from the view group. Views are removed
		// in reverse (LIFO).
		virtual void remove_view();

		Path get_path() const;
		void set_path(Path path);
		// Atomically reloads current path.
		// Equivalent to calling set_path(get_path()) but atomic.
		void reload_path();

		// Warning: the returned std::vector causes a data race
		//          with add_view and remove_view.
		const List_dir_vector& get_views() const;

		// Warning: the View pointed to by the returned pointer gets
		//          deleted every time the cursor gets changed
		//          (set_path/reload_path changes the cursor as well).
		const View* get_preview() const;

		Path get_cursor_path() const;
		// See handlers/List_dir.h for Cursor_search_direction
		void set_cursor(const Path& filename,
				List_dir::Cursor_search_direction dir =
					List_dir::Cursor_search_direction::begin);

		void advance_cursor(Dir_vector::difference_type d);
		void rewind_cursor(List_dir::Cursor_position p);

	private:
		void update_cursors(Path path);
		void set_cursor_path(const Path& p);

		void update_paths(Path path);
		void update_active_cursor();
		bool can_set_cursor();

		// Constructs View::Ready_guard and sets the view's path.
		void ready_view(View& v, const Path& path);

		void create_preview(const Path& path, bool set_cpath);
		void destroy_preview();
		// If the user is scrolling the cursor too fast, don't
		// create the preview immediately. Instead, wait for
		// m_preview_delay milliseconds whilst checking for
		// interrupts.
		void delay_preview();
		void delay_create_preview();
	};
}

#endif // HAWK_TAB_H
