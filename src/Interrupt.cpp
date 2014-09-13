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

#include "Interrupt.h"

namespace hawk {

thread_local Interrupt_flag _soft_iflag;
thread_local Interrupt_flag _hard_iflag;

void Interrupt_flag::set()
{
	m_flag.store(true, std::memory_order_relaxed);
}

void Interrupt_flag::clear()
{
	m_flag.store(false, std::memory_order_relaxed);
}

bool Interrupt_flag::is_set() const
{
	return m_flag.load(std::memory_order_relaxed);
}

void soft_interruption_point()
{
	if (_soft_iflag.is_set())
	{
		_soft_iflag.clear();
		throw Soft_thread_interrupt();
	}
}

void hard_interruption_point()
{
	if (_hard_iflag.is_set())
	{
		_hard_iflag.clear();
		throw Hard_thread_interrupt();
	}
}

} // namespace hawk
