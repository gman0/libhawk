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
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include "Path.h"
#include "Type_factory.h"
#include "Dir_cache.h"
#include "Interruptible_thread.h"
#include "handlers/List_dir.h"

namespace hawk {
	class Cursor_cache;
	class View;
	class List_dir;

	/// Exception safety
	//  User's code can safely throw exceptions with these guarantees:
	//   * throwing an exception during View_group initialization (ie. when
	//     calling build_views - where the user code being the
	//     constructor of a class inherting from View, set_path virtual
	//     method and ready pure virtual method) causes the View_group to cease
	//     its construction,
	//   * throwing an exception while creating a preview
	//     (ie. constructor, set_path, ready) causes the preview
	//     to cease its construction,
	//   * throwing an exception while setting View_group path by calling
	//     set_path (which subsequently calls set_path of every View) causes
	//     the path to be reset to its previous value. If this call fails
	//     again by throwing an exception, the parent path of current path
	//     will be used - this will continue in a loop until call to set_path
	//     succeeds (thus the path may end up being "/").
	//
	//  Any exception thrown in any of these scenarios will be rethrown back
	//  to the user.

	// A note about View building: when calling build_views (during
	// View_group construction), View constructors are called in forward order
	// but their ready methods are called in reverse.
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

		List_dir_vector m_views;
		View_ptr m_preview;
		std::chrono::milliseconds m_preview_delay;
		std::chrono::time_point<
			std::chrono::steady_clock> m_preview_timestamp;

		const Type_factory& m_type_factory;
		Type_factory::Handler m_list_dir_closure;

		Interruptible_thread m_tasking_thread;
		struct Tasking
		{
			std::condition_variable cv;
			std::mutex m;
			bool ready_for_tasking;
			std::atomic<bool> global;
			std::function<void()> task;

			Exception_handler exception_handler;

			Tasking(const Exception_handler& eh)
				:
				  ready_for_tasking{true},
				  global{false},
				  exception_handler{eh}
			{}

			void run_task(std::unique_lock<std::mutex>& lk,
						  std::function<void()>&& f);
			void operator()();
		} m_tasking;

	public:
		View_group(
				const Path& p, int nviews, const Type_factory& tf,
				const Exception_handler& eh,
				const Type_factory::Handler& list_dir_closure,
				std::chrono::milliseconds preview_delay)
			:
			  m_path{p},
			  m_preview_delay{preview_delay},
			  m_type_factory{tf},
			  m_list_dir_closure{list_dir_closure},
			  m_tasking{eh}
		{
			build_views(--nviews);
			m_tasking_thread = Interruptible_thread {std::ref(m_tasking)};
		}

		~View_group();

		View_group(const View_group&) = delete;
		View_group& operator=(const View_group&) = delete;

		Path get_path() const;
		void set_path(Path path);
		// Atomically reloads current path.
		// Equivalent to calling set_path(get_path()) but atomic.
		void reload_path();

		const List_dir_vector& get_views() const;

		void set_cursor(Dir_cursor cursor);
		// See handlers/List_dir.h for Cursor_search_direction
		void set_cursor(const Path& filename,
				List_dir::Cursor_search_direction dir =
					List_dir::Cursor_search_direction::begin);

	private:
		void build_views(int nviews);
		void instantiate_views(int nviews);
		void initialize_views(int nviews);

		void update_paths(Path path);
		void update_active_cursor();
		// Used when calling set_cursor(). Returns true if
		// the cursor can be safely set.
		bool can_set_cursor();

		void add_view(const Type_factory::Handler& closure);
		// Sets view's path and calls its ready().
		void ready_view(View& v, const Path& path);

		void create_preview(const Path& path);
		void destroy_preview();

		void task_load_path(const Path& path);
		void task_create_preview(const Path& path);
	};
}

#endif // HAWK_TAB_H
