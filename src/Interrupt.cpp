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

#include "Interrupt.h"

namespace hawk {

thread_local Interrupt_flag _soft_iflag;
thread_local Interrupt_flag _hard_iflag;

void Interrupt_flag::set()
{
	std::lock_guard<std::mutex> lk {m};
	flag = true;
}

void Interrupt_flag::clear()
{
	std::lock_guard<std::mutex> lk {m};
	flag = false;
}

void soft_interruption_point()
{
	std::lock_guard<std::mutex> lk {_soft_iflag.m};
	if (_soft_iflag.flag)
	{
		_soft_iflag.flag = false;
		throw Soft_thread_interrupt();
	}
}

void hard_interruption_point()
{
	std::lock_guard<std::mutex> lk {_hard_iflag.m};
	if (_hard_iflag.flag)
	{
		_hard_iflag.flag = false;
		throw Hard_thread_interrupt();
	}
}

} // namespace hawk
