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

#ifndef HAWK_INTERRUPT_H
#define HAWK_INTERRUPT_H

#include <atomic>

namespace hawk {
	class Interrupt_flag
	{
	private:
		std::atomic<bool> m_flag;

	public:
		void set();
		bool is_set() const;
	};

	extern thread_local Interrupt_flag _soft_iflag;
	extern thread_local Interrupt_flag _hard_iflag;

	// These two classes are used as exception types.
	class Soft_thread_interrupt {};
	class Hard_thread_interrupt {};

	void soft_interruption_point();
	void hard_interruption_point();
}

#endif // HAWK_INTERRUPT_H
