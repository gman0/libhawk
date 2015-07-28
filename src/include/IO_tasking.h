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

#ifndef HAWK_IO_TASKING_H
#define HAWK_IO_TASKING_H

#include <memory>
#include <chrono>
#include <deque>
#include <functional>
#include "Path.h"
#include "Filesystem.h"
#include "Interruptible_thread.h"

namespace hawk {
	struct Task_progress
	{
		const Path& src;
		const Path& dst;
	};

	struct File_progress
	{
		uintmax_t total;
		uintmax_t offset;
		uintmax_t rate;
		std::chrono::seconds eta_end;
	};

	class IO_task;

	using Task_progress_monitor =
			std::function<void(IO_task*, const Task_progress&) noexcept>;
	using File_progress_monitor =
			std::function<void(IO_task*, const File_progress&) noexcept>;

	void set_io_task_callbacks(
			Task_progress_monitor&& tmon, File_progress_monitor&& fmon);

	class IO_task_error
	{
	private:
		Path src;
		Path dst;
		int err;

	public:
		IO_task_error(const Path& s, const Path& d, int e)
			: src{s}, dst{d}, err{e} {}

		const Path& get_source() const { return src; }
		const Path& get_destination() const { return dst; }
		int get_errno() const { return err; }
	};

	struct IO_task_fatal : public Filesystem_error
	{
		using Filesystem_error::Filesystem_error;
	};

	class IO_task
	{
	public:
		enum class Status {preparing, pending, finished, failed, paused};

		struct Context
		{
			uintmax_t offset;
			Recursive_directory_iterator dir_iter;
			std::chrono::steady_clock::time_point start;
		};

		struct Item
		{
			Path src;
			Path dst;

			Item(const Path& s) : src{s} {}
			Item(const Path& s, const Path& d) : src{s}, dst{d} {}
		};

	protected:
		Path m_dst;

		bool m_check_avail_space;
		bool m_deref_symlinks;
		bool m_update_symlinks;

		Context m_ctx;
		std::deque<Item> m_items;

	private:
		uintmax_t m_total;
		std::chrono::steady_clock::time_point m_start;

		Interruptible_thread m_tasking_thread;
		Status m_status;

	public:
		IO_task(const Path& src, const Path& dst,
				bool dereference_symlinks, bool update_symlinks,
				bool check_avail_space = true);
		IO_task(const std::vector<Path>& srcs, const Path& dst,
				bool dereference_symlinks, bool update_symlinks,
				bool check_avail_space = true);

		virtual ~IO_task() = default;

		void resume();
		void pause();
		void cancel();

		// The result of this method may well be a data race
		// if the task hasn't finished yet.
		Status get_status() const;

		std::chrono::steady_clock::time_point time_started() const;
		uintmax_t total_size() const;

	protected:
		void start_tasking();

		void insert_item(const Item& item);
		const Item& current_item() const;

		virtual void reset_context();

		void handle_symlink(const Path& src,
							const Path* abs_deref, const Path& dst,
							std::deque<Item>& items);

		virtual void traverse_directory(
				Recursive_directory_iterator& dir_iter, Item& i) = 0;
		void traverse_directory(
				Recursive_directory_iterator it, const Path& i,
				std::function<bool(const Stat&, const Path&)>&& fn);

		// The task ends with Status::failed if it encounters ENOENT error:
		// the file/directory no longer exists, the data being processed
		// are no longer consistent - the task cannot be completed.
		template <typename FSException>
		void may_fail(const FSException& e)
		{
			if (e.get_errno() == ENOENT)
				throw IO_task_fatal {e.get_source(), ENOENT};
			else
				on_error(e);
		}
		void may_fail(const Path& src, const Path& dst, int err) const;

		bool has_enough_space();
		virtual uintmax_t accumulate_file_size(
				const Stat& st, const Path& p,
				Recursive_directory_iterator it,
				bool resumed_state, uintmax_t total, uintmax_t avail,
				std::deque<Path>& srcs) = 0;

		void process_file(const Item& i);
		virtual void process_file(const Path& src, const Path& dst) = 0;
		virtual void process_directory(Item& i) = 0;
		virtual void process_symlink(const Path& target, const Path& linkpath,
									 const Path& src) = 0;

		// These methods below (on_*) are to be implemented by the user.

		virtual void on_error(const IO_task_error& e) const noexcept = 0;
		virtual void on_error(const Filesystem_error& e) const noexcept = 0;
		virtual void on_status_change(Status st) const noexcept = 0;

	private:
		void dispatch_item(Item& i);
		void tasking();
		void set_status(Status st);
	};

	class IO_task_copy : public IO_task
	{
	public:
		using IO_task::IO_task;

	private:
		virtual uintmax_t accumulate_file_size(
				const Stat& st, const Path& p,
				Recursive_directory_iterator it,
				bool resumed_state, uintmax_t total, uintmax_t avail,
				std::deque<Path>& srcs);

		virtual void process_file(const Path& src, const Path& dst);
		virtual void process_directory(Item& i);
		virtual void process_symlink(const Path& target, const Path& linkpath,
									 const Path& src);
		virtual void traverse_directory(
				Recursive_directory_iterator& dir_iter, Item& i);
	};

	class IO_task_move : public IO_task
	{
	private:
		Stat m_dst_st;
		int m_prev_level;
		Path m_prev_path;

	public:
		IO_task_move(const Path& src, const Path& dst, bool update_symlinks);
		IO_task_move(const std::vector<Path>& srcs, const Path& dst,
					 bool update_symlinks);

	private:
		virtual void traverse_directory(
				Recursive_directory_iterator& dir_iter, Item& i);
		void update_symlinks(const Path& src, const Path& dst);

		virtual uintmax_t accumulate_file_size(
				const Stat& st, const Path& p,
				Recursive_directory_iterator it,
				bool resumed_state, uintmax_t total, uintmax_t avail,
				std::deque<Path>& srcs);

		virtual void process_file(const Path& src, const Path& dst);
		virtual void process_directory(Item& i);
		virtual void process_symlink(const Path& target, const Path& linkpath,
									 const Path& src);

		virtual void reset_context();
	};

	class IO_task_remove : public IO_task
	{
	public:
		explicit IO_task_remove(const Path& p);
		explicit IO_task_remove(const std::vector<Path>& pvec);

	private:
		virtual uintmax_t accumulate_file_size(
				const Stat& st, const Path& p,
				Recursive_directory_iterator it,
				bool resumed_state, uintmax_t total, uintmax_t avail,
				std::deque<Path>& srcs);

		virtual void traverse_directory(
				Recursive_directory_iterator& dir_iter, Item& i);

		virtual void process_file(const Path& src, const Path& dst);
		virtual void process_directory(Item& i);
		virtual void process_symlink(const Path& target, const Path& linkpath,
									 const Path& src);
	};
}

#endif // HAWK_IO_TASKING_H
