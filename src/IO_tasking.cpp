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

#include <cstdio>
#include <algorithm>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include "Interruptible_thread.h"
#include "Filesystem.h"
#include "IO_tasking.h"

namespace hawk {

using Time_point = std::chrono::steady_clock::time_point;

namespace {

static struct
{
	Task_progress_monitor tmon;
	File_progress_monitor fmon;
} s_monitor_callbacks;

bool is_in_parent_path(const Path& base, const Path& child_path)
{
	unsigned b_sz = base.length();
	if (b_sz >= child_path.length()) return false;

	return memcmp(base.c_str(), child_path.c_str(), b_sz) == 0;
}

class File
{
private:
	int m_fd;

public:
	File(const Path& file, int flags, mode_t mode)
	{
		m_fd = open64(file.c_str(), flags, mode);
		if (m_fd == -1) throw Filesystem_error {file, errno};
	}

	~File() { close(m_fd); }

	int get_fd() const noexcept { return m_fd; }

	off64_t read(char* buf, size_t sz) noexcept
	{
		return ::read(m_fd, buf, sz);
	}

	ssize_t write(char* buf, size_t sz) noexcept
	{
		return ::write(m_fd, buf, sz);
	}

	void seek(off64_t offset) noexcept
	{
		lseek64(m_fd, offset, SEEK_SET);
	}

	off64_t tell() const noexcept
	{
		return lseek64(m_fd, 0, SEEK_CUR);
	}
};

void update_progress(
		IO_task* parent, uintmax_t bytes_read, uintmax_t offset,
		uintmax_t total_size, const Time_point& start, Time_point& last_update)
{
	auto now = std::chrono::steady_clock::now();
	if (now - last_update > std::chrono::seconds {1})
	{
		last_update = now;
		std::chrono::seconds time_elapsed =
				std::chrono::duration_cast<
					std::chrono::seconds>(now - start);

		File_progress progress;
		progress.total = total_size;
		progress.offset = offset;
		progress.rate = bytes_read / time_elapsed.count();
		progress.eta_end =
				(total_size - bytes_read) * time_elapsed / bytes_read;

		s_monitor_callbacks.fmon(parent, progress);
		last_update = now;

		hard_interruption_point();
	}
}

void copy_file(IO_task* parent, IO_task::Context& ctx,
			   const Path& src, const Path& dst)
{
	// Prepare for copying.

	if (exists(dst) && ctx.offset == 0)
		throw Filesystem_error {dst, EEXIST};

	uintmax_t sz = file_size(src);

	File src_file {src, O_RDONLY, 0440};
	File dst_file {dst, O_WRONLY | O_CREAT, 0666};

	if (sz == 0) return;

	if (ctx.offset == 0)
	{
		/*
		 * Calling posix_fallocate on filesystems that don't support
		 * preallocation is bullshit:
		 * https://sourceware.org/bugzilla/show_bug.cgi?id=18515
		 * even though there's already a fix for the POSIX spec.:
		 * http://pubs.opengroup.org/onlinepubs/9699919799//functions/posix_fallocate.html#tag_16_366_05
		 * using EINVAL as a preallocation-not-supported error.
		 *
		 * TODO: uncomment this code somewhere in the future
		 *
		int err = 0;
		err = posix_fallocate64(dst_file.get_fd(), 0, sz);

		if (err && err != EINVAL) throw IO_task_fatal {dst, err};
		*/
	}
	else
	{
		src_file.seek(ctx.offset);
		dst_file.seek(ctx.offset);

		// Compensate for bytes already copied before resuming
		// so that we'll get correct ETA and rate calculations.
		sz -= ctx.offset;
	}

	int err = posix_fadvise64(src_file.get_fd(), ctx.offset, 0,
							  POSIX_FADV_SEQUENTIAL);
	if (err) throw IO_task_fatal {src, err};

	constexpr unsigned buf_sz  = 8192;
	char buf[buf_sz];

	// Set timers.

	ctx.start = std::chrono::steady_clock::now();
	auto last_update = std::chrono::steady_clock::now();
	uintmax_t bytes_read = 0;

	// Commence copying!

	for (;;)
	{
		off64_t read_sz = src_file.read(buf, buf_sz);
		if (read_sz == 0)
			break;

		if (read_sz == -1)
			throw Filesystem_error {src, errno};

		if (dst_file.write(buf, read_sz) != read_sz)
			throw Filesystem_error {dst, errno};

		bytes_read += read_sz;
		ctx.offset += read_sz;
		update_progress(parent, bytes_read, ctx.offset, sz,
						ctx.start, last_update);
	}
}

bool same_dev(const Stat& src, const Stat& dst)
{
	return src.st_dev == dst.st_dev;
}

void rename_move(const Path& src, const Path& dst)
{
	if (rename(src.c_str(), dst.c_str()) != 0)
		throw IO_task_error {src, dst, errno};
}

Path transform_symlink_path(
		const Path& link_target, const Path& src, const Path& dst,
		const Path& base_dst, std::string::size_type top_len)
{
	Path s = src.parent_path();
	if (is_in_parent_path(s, link_target))
		return dst.parent_path() / (link_target.c_str() + s.length());
	else if (is_in_parent_path(link_target, s))
		return base_dst / (link_target.c_str() + top_len);

	return link_target;
}

} // unnamed-namespace

void set_io_task_callbacks(Task_progress_monitor&& tmon,
						   File_progress_monitor&& fmon)
{
	s_monitor_callbacks.tmon = std::move(tmon);
	s_monitor_callbacks.fmon = std::move(fmon);
}

// IO_task implementation

IO_task::IO_task(const Path& src, const Path& dst,
				 bool dereference_symlinks, bool update_symlinks,
				 bool check_avail_space)
	:
	  m_dst{dst},
	  m_check_avail_space{check_avail_space},
	  m_deref_symlinks{dereference_symlinks},
	  m_update_symlinks{update_symlinks},
	  m_total{0}
{
	m_items.emplace_back(src);
	m_ctx.offset = 0;
}

IO_task::IO_task(const std::vector<Path>& srcs, const Path& dst,
				 bool dereference_symlinks, bool update_symlinks,
				 bool check_avail_space)
	:
	  m_dst{dst},
	  m_check_avail_space{check_avail_space},
	  m_deref_symlinks{dereference_symlinks},
	  m_update_symlinks{update_symlinks},
	  m_total{0}
{
	for (const Path& p : srcs)
		m_items.emplace_back(p);

	m_ctx.offset = 0;
}

void IO_task::resume()
{
	start_tasking();
}

void IO_task::pause()
{
	m_tasking_thread.hard_interrupt();
	m_tasking_thread.join();

	set_status(Status::paused);
}

void IO_task::cancel()
{
	m_tasking_thread.hard_interrupt();
	m_tasking_thread.join();

	set_status(Status::finished);
}

IO_task::Status IO_task::get_status() const
{
	return m_status;
}

std::chrono::steady_clock::time_point IO_task::time_started() const
{
	return m_start;
}

uintmax_t IO_task::total_size() const
{
	return m_total;
}

void IO_task::start_tasking()
{
	m_tasking_thread = Interruptible_thread {[&]{
		try {
			tasking();
		} catch (const IO_task_fatal& e) {
			on_error(static_cast<const Filesystem_error&>(e));
			set_status(Status::failed);
		}
	}};
}

void IO_task::insert_item(const IO_task::Item& item)
{
	m_items.push_front(item);
}

const IO_task::Item& IO_task::current_item() const
{
	return m_items.front();
}

void IO_task::reset_context()
{
	m_ctx.offset = 0;
	m_ctx.dir_iter = Recursive_directory_iterator {};
}

void IO_task::tasking()
{
	if (m_check_avail_space)
	{
		set_status(Status::preparing);

		if (!has_enough_space())
		{
			on_error(Filesystem_error {m_dst, ENOSPC});
			set_status(Status::failed);

			return;
		}
	}

	set_status(Status::pending);
	m_start = std::chrono::steady_clock::now();

	while (!m_items.empty())
	{
		hard_interruption_point();
		auto head = m_items.begin();

		try { dispatch_item(m_items.front()); }
		catch (const IO_task_error& e) { may_fail(e); }
		catch (const Filesystem_error& e) { may_fail(e); }

		m_items.erase(head);
		reset_context();
	}

	set_status(Status::finished);
}

void IO_task::set_status(IO_task::Status st)
{
	m_status = st;
	on_status_change(st);
}

void IO_task::handle_symlink(const Path& src, const Path* abs_deref,
							 const Path& dst, std::deque<IO_task::Item>& items)
{
	Path link_target = read_symlink(src);
	Path new_target;
	if (m_update_symlinks && link_target.is_absolute())
	{
		new_target =
				transform_symlink_path(link_target, src, dst, m_dst,
									   items.front().src.length());
	}
	else
		new_target = link_target;

	assert(!(m_deref_symlinks && !abs_deref));
	if (m_deref_symlinks && !is_in_parent_path(*abs_deref, src))
	{
		items.emplace_front(*abs_deref, dst);
		return;
	}

	process_symlink(new_target, dst, src);
}

void IO_task::traverse_directory(Recursive_directory_iterator it, const Path& i,
		std::function<bool(const Stat&, const Path&)>&& fn)
{
	while (!it.at_end())
	{
		hard_interruption_point();

		Path p = i / *it;
		Stat st = hawk::symlink_status(p);

		if ( !(is_symlink(st) && is_in_parent_path(canonical(p, "/"), p)) )
		{
			if (!fn(st, i / *it))
				break;
		}

		try {
			if (is_symlink(st))
				it.orthogonal_increment();
			else
				++it;
		} catch (const Filesystem_error&) {
			it.orthogonal_increment();
		}
	}
}

void IO_task::may_fail(const Path& src, const Path& dst, int err) const
{
	if (err == ENOENT)
		throw IO_task_fatal {src, err};
	else
		on_error(IO_task_error {src, dst, err});
}

void IO_task::dispatch_item(Item& i)
{
	s_monitor_callbacks.tmon(this, Task_progress {i.src, i.dst});

	Stat st = hawk::symlink_status(i.src);

	if (is_symlink(st))
	{
		Path abs_deref = canonical(i.src, "/");
		handle_symlink(i.src, &abs_deref, i.dst, m_items);

		return;
	}

	if (i.dst.empty())
	{
		if (exists(m_dst) && is_directory(m_dst))
			i.dst = m_dst / i.src.filename();
		else
			i.dst = m_dst;
	}

	if (is_regular_file(st))
		process_file(i);
	else if (is_directory(st))
		process_directory(i);
}

bool IO_task::has_enough_space()
{
	uintmax_t avail;
	if (exists(m_dst))
		avail = space(m_dst).available;
	else
		avail = space(m_dst.parent_path()).available;

	uintmax_t total = 0;

	std::deque<Path> srcs;
	for (const Item& i : m_items)
		srcs.push_back(i.src);

	bool dir_resumed_state = false;
	Recursive_directory_iterator dir_iter;
	if (!m_ctx.dir_iter.at_end())
	{
		dir_iter = Recursive_directory_iterator {srcs.front()};
		dir_iter.advance(m_ctx.dir_iter);
		dir_resumed_state = true;

		if (m_ctx.offset != 0)
		{
			std::advance(dir_iter, 2);
			// 2: m_ctx.dir_iter hasn't moved to the current file yet.
			// Also, we want to skip the current file.
		}
	} else if (m_ctx.offset != 0)
		srcs.pop_front();

	while (!srcs.empty())
	{
		hard_interruption_point();

		if (total >= avail)
			return false;

		auto head = srcs.begin();
		const Path& src = *head;
		Stat st = hawk::symlink_status(src);

		try {
			total = accumulate_file_size(st, src, dir_iter, dir_resumed_state,
										 total, avail, srcs);
		} catch (...) {}
		dir_resumed_state = false;

		srcs.erase(head);
	}

	m_total = total;

	return true;
}

void IO_task::process_file(const IO_task::Item& i)
{
	process_file(i.src, i.dst);
}

// IO_task_copy implementation

uintmax_t IO_task_copy::accumulate_file_size(
		const Stat& st, const Path& p, Recursive_directory_iterator dir_iter,
		bool resumed_state, uintmax_t total, uintmax_t avail,
		std::deque<Path>& srcs)
{
	if (is_regular_file(st))
		return total + file_size(st);
	else if (is_directory(st))
	{
		if (!resumed_state)
			dir_iter = Recursive_directory_iterator {p};

		IO_task::traverse_directory(dir_iter, p,
			[&](const Stat& st, const Path& i) {
				if (is_regular_file(st))
				{
					total += file_size(st);
					if (total >= avail)
						return false;
				}
				else if (m_deref_symlinks && is_symlink(st))
				{
					Path deref = canonical(i, "/");

					if (!is_in_parent_path(i, deref))
						srcs.push_back(deref);
				}

				return true;
			});
	}
	else if (is_symlink(st) && m_deref_symlinks)
	{
		Path deref = canonical(p, "/");
		if (!is_in_parent_path(p, deref))
			srcs.push_back(deref);
	}

	return total;
}

void IO_task_copy::traverse_directory(
		Recursive_directory_iterator& dir_iter, Item& i)
{
	while (!dir_iter.at_end())
	{
		hard_interruption_point();

		Path p = *dir_iter;
		Path src = i.src / p;
		Path dst = i.dst / p;
		Stat st = symlink_status(src);

		s_monitor_callbacks.tmon(this, Task_progress {src, dst});

		try {
			if (is_symlink(st) && (
					!m_deref_symlinks || is_in_parent_path(i.src, src)))
			{
				dir_iter.orthogonal_increment();
			}
			else
				++dir_iter;
		} catch (const Filesystem_error& e) {
			may_fail(src, dst, e.get_errno());
			dir_iter.orthogonal_increment();
		}

		if (is_directory(st))
		{
			int err = 0;
			create_directory(dst, err);

			// Ignore existing directory.
			if (err && err != EEXIST)
				on_error(IO_task_error {src, dst, err});
		}
		else if (is_regular_file(st))
		{
			try { process_file(src, dst); }
			catch (const Filesystem_error& e) {
				may_fail(src, dst, e.get_errno());
				m_ctx.offset = 0;

				return;
			}

			// Reset offset after copy_file() has finished.
			m_ctx.offset = 0;
		}
		else if (is_symlink(st))
		{
			Path abs_deref = canonical(src, "/");
			handle_symlink(src, &abs_deref, dst, m_items);
		}
	}
}

void IO_task_copy::process_directory(Item& i)
{
	Recursive_directory_iterator& dir_iter = m_ctx.dir_iter;

	if (dir_iter.at_end())
	{
		dir_iter = Recursive_directory_iterator {i.src};
		create_directory(i.dst);
	}

	traverse_directory(dir_iter, i);
	copy_permissions(i.src, i.dst);
}

void IO_task_copy::process_file(const Path& src, const Path& dst)
{
	copy_file(this, m_ctx, src, dst);
	copy_permissions(src, dst);
}

void IO_task_copy::process_symlink(const Path& target, const Path& linkpath,
								   const Path&)
{
	int err;
	create_symlink(target, linkpath, err);

	if (err)
	{
		if (err != ENOENT)
			throw Filesystem_error {linkpath, err};
		else
			return;
	}
}

// IO_task_move implementation

IO_task_move::IO_task_move(const Path& src, const Path& dst,
						   bool update_symlinks)
	: IO_task{src, dst, false, update_symlinks}, m_prev_level{-1}
{
	m_dst_st = (exists(m_dst)) ? hawk::status(m_dst)
							   : hawk::status(m_dst.parent_path());
}

IO_task_move::IO_task_move(const std::vector<Path>& srcs, const Path& dst,
						   bool update_symlinks)
	: IO_task{srcs, dst, false, update_symlinks}, m_prev_level{-1}
{
	m_dst_st = (exists(m_dst)) ? hawk::status(m_dst)
							   : hawk::status(m_dst.parent_path());
}

void IO_task_move::traverse_directory(
		Recursive_directory_iterator& dir_iter, IO_task::Item& i)
{
	while (!dir_iter.at_end())
	{
		if (m_prev_level > dir_iter.level())
		{
			remove_ndirectories(m_prev_level - dir_iter.level() - 1,
								std::move(m_prev_path));
			m_prev_level = -1;
		}

		hard_interruption_point();

		Path p = *dir_iter;
		Path src = i.src / p;
		Path dst = i.dst / p;
		Stat st = symlink_status(src);
		m_prev_level = dir_iter.level();

		s_monitor_callbacks.tmon(this, Task_progress {src, dst});

		// Increment the iterator.

		try {
			if (is_directory(st))
				++dir_iter;
			else
				dir_iter.orthogonal_increment();
		} catch (const Filesystem_error& e) {
			may_fail(src, dst, e.get_errno());
			dir_iter.orthogonal_increment();
		}

		// Handle the direrectory/file/symlink.

		if (is_directory(st))
		{
			int err;
			create_directory(dst, err);

			if (err && err != EEXIST)
			{
				on_error(IO_task_error {src, dst, err});
				continue;
			}

			m_prev_path = src;
		}
		else
		{
			if (is_regular_file(st))
			{
				try { process_file(src, dst); }
				catch (const Filesystem_error& e) {
					may_fail(src, dst, e.get_errno());
					m_ctx.offset = 0;

					continue;
				}

				m_ctx.offset = 0;
			}
			else if (is_symlink(st))
				handle_symlink(src, nullptr, dst, m_items);
		}
	}

	copy_permissions(i.src, i.dst);
	remove_ndirectories(m_prev_level, std::move(m_prev_path));
}

void IO_task_move::update_symlinks(const Path& src, const Path& dst)
{
	Recursive_directory_iterator it {dst};
	while (!it.at_end())
	{
		Path p = *it;
		Path abs_dst = dst / p;

		if (is_symlink(symlink_status(abs_dst)))
		{
			it.orthogonal_increment();

			Path target = read_symlink(abs_dst);
			if (!target.is_absolute())
				continue;

			Path new_target = transform_symlink_path(
						target, src / p, abs_dst, dst, src.length());

			remove_file(abs_dst);
			create_symlink(new_target, abs_dst);
		}
		else
			++it;
	}
}

uintmax_t IO_task_move::accumulate_file_size(
		const Stat& st, const Path& p, Recursive_directory_iterator dir_iter,
		bool resumed_state, uintmax_t total, uintmax_t avail,
		std::deque<Path>&)
{
	if (same_dev(st, m_dst_st))
		return total;

	if (is_regular_file(st))
		return total + file_size(st);

	if (is_directory(st))
	{
		if (!resumed_state)
			dir_iter = Recursive_directory_iterator {p};

		IO_task::traverse_directory(dir_iter, p,
			[&](const Stat& st, const Path&) {
				if (is_regular_file(st))
				{
					total += file_size(st);
					if (total >= avail)
						return false;
				}

				return true;
			});
	}

	return total;
}

void IO_task_move::process_directory(Item& i)
{
	if (same_dev(hawk::status(i.src), m_dst_st))
	{
		rename_move(i.src, i.dst);

		if (m_update_symlinks)
			update_symlinks(i.src, i.dst);

		return;
	}

	Recursive_directory_iterator& dir_iter = m_ctx.dir_iter;

	if (dir_iter.at_end())
	{
		dir_iter = Recursive_directory_iterator {i.src};
		create_directory(i.dst);
	}

	traverse_directory(dir_iter, i);
}

void IO_task_move::process_file(const Path& src, const Path& dst)
{
	if (same_dev(hawk::status(src), m_dst_st))
		rename_move(src, dst);
	else
	{
		copy_file(this, m_ctx, src, dst);
		copy_permissions(src, dst);

		if (unlink(src.c_str()) != 0)
			throw IO_task_error {src, dst, errno};
	}
}

void IO_task_move::process_symlink(const Path& target, const Path& linkpath,
								   const Path& src)
{
	int err;
	create_symlink(target, linkpath, err);

	if (err)
	{
		if (err != ENOENT)
			throw Filesystem_error {linkpath, err};
		else
			return;
	}

	if (unlink(src.c_str()) != 0)
		throw IO_task_error {target, linkpath, errno};
}

void IO_task_move::reset_context()
{
	IO_task::reset_context();
	m_prev_level = -1;
	m_prev_path.clear();
}

// IO_task_remove implementation

IO_task_remove::IO_task_remove(const Path& p)
	: IO_task{p, Path(), false, false, false}
{}

IO_task_remove::IO_task_remove(const std::vector<Path>& pvec)
	: IO_task{pvec, Path(), false, false, false}
{}

uintmax_t IO_task_remove::accumulate_file_size(
		const Stat&, const Path&, Recursive_directory_iterator,
		bool, uintmax_t, uintmax_t, std::deque<Path>&)
{
	return 0;
}

void IO_task_remove::process_file(const Path& src, const Path&)
{
	remove_file(src);
}

void IO_task_remove::traverse_directory(
		Recursive_directory_iterator&, IO_task::Item&)
{}

// The same implementation as in remove_recursively() but interruptible.
void IO_task_remove::process_directory(IO_task::Item& i)
{
	Path prev_path;
	int prev_level = 0;

	Recursive_directory_iterator it {i.src};
	while (!it.at_end())
	{
		if (prev_level > it.level())
		{
			// We've left a directory/directories which means
			// they're now empty and should be safe to remove.
			remove_ndirectories(
						prev_level - it.level() - 1, std::move(prev_path));
		}

		try { hard_interruption_point(); }
		catch (...) { reset_context(); throw; }

		Path p = i.src / *it;
		Stat st = symlink_status(p);
		prev_level = it.level();

		s_monitor_callbacks.tmon(this, Task_progress {p, Path()});

		if (is_symlink(st))
			it.orthogonal_increment(); // Don't iterate through symlinks.
		else
			++it;

		if (!is_directory(st))
			remove_file(p);
		else
			prev_path = p;
	}

	remove_ndirectories(prev_level, std::move(prev_path));
}

void IO_task_remove::process_symlink(const Path&, const Path&, const Path& src)
{
	remove_file(src);
}

} // namespace hawk
