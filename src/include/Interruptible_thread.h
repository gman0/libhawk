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

#ifndef HAWK_INTERRUPTIBLE_THREAD_H
#define HAWK_INTERRUPTIBLE_THREAD_H

#include <thread>
#include <future>
#include "Interrupt.h"

namespace hawk {
	class Interruptible_thread
	{
	private:
		std::thread m_thread;
		Interrupt_flag* m_hard_iflag;
		Interrupt_flag* m_soft_iflag;

	public:
		Interruptible_thread() {}

		template <typename Function, typename... Args>
		explicit Interruptible_thread(Function&& f, Args&&... args)
		{
			std::promise<Interrupt_flag*> hard;
			std::promise<Interrupt_flag*> soft;

			m_thread = std::thread([&, f]{
				hard.set_value(&_hard_iflag);
				soft.set_value(&_soft_iflag);

				try {
					f(std::forward<Args>(args)...);
				} catch (const Hard_thread_interrupt&) {}
			});

			m_hard_iflag = hard.get_future().get();
			m_soft_iflag = soft.get_future().get();
		}

		Interruptible_thread(Interruptible_thread&& ithread) noexcept;
		Interruptible_thread& operator=(
				Interruptible_thread&& ithread) noexcept;

		~Interruptible_thread();

		Interruptible_thread(const Interruptible_thread&) = delete;
		Interruptible_thread& operator=(const Interruptible_thread&) = delete;

		void join();
		bool joinable() const;

		void hard_interrupt();
		void soft_interrupt();
	};
}

#endif // HAWK_INTERRUPTIBLE_THREAD_H
