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
#include "Path.h"

namespace hawk {
	struct Task_item
	{
		Path source;
		Path destination;
		size_t size;
		std::chrono::steady_clock::time_point start;
	};

	struct Task_progress
	{
		size_t offset;
		size_t rate;
		std::chrono::seconds eta_end;
	};

	using Exception_handler = std::function<
		void(std::exception_ptr) noexcept>;
	using Item_monitor = void(*)(const Task_item&);
	using Progress_monitor = void(*)(const Task_progress&);
	void set_io_task_callbacks(Exception_handler&& eh, Item_monitor&& imon,
							   Progress_monitor&& pmon);

	// These functions can be called only from one thread!
	void copy(const Path& source, const Path& destination);
	void move(const Path& source, const Path& destination);
	void cancel_task(const Path& source, const Path& destination);
}

#endif // HAWK_IO_TASKING_H
