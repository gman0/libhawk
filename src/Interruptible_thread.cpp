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

#include <atomic>
#include "Interruptible_thread.h"

namespace hawk {

Interruptible_thread::Interruptible_thread(
		Interruptible_thread&& ithread) noexcept
	:
	  m_thread{std::move(ithread.m_thread)},
	  m_hard_iflag{ithread.m_hard_iflag},
	  m_soft_iflag{ithread.m_soft_iflag}
{
	ithread.m_hard_iflag = nullptr;
	ithread.m_soft_iflag = nullptr;
}

Interruptible_thread& Interruptible_thread::operator=(
		Interruptible_thread&& ithread) noexcept
{
	if (this == &ithread)
		return *this;

	m_thread = std::move(ithread.m_thread);
	m_hard_iflag = ithread.m_hard_iflag;
	m_soft_iflag = ithread.m_soft_iflag;

	ithread.m_hard_iflag = nullptr;
	ithread.m_soft_iflag = nullptr;

	return *this;
}

Interruptible_thread::~Interruptible_thread()
{
	if (m_thread.joinable())
		join();
}

bool Interruptible_thread::joinable() const
{
	return m_thread.joinable();
}

void Interruptible_thread::join()
{
	m_thread.join();
	m_hard_iflag = nullptr;
	m_soft_iflag = nullptr;
}

void Interruptible_thread::hard_interrupt()
{
	if (m_hard_iflag)
		m_hard_iflag->set();
}

void Interruptible_thread::soft_interrupt()
{
	if (m_soft_iflag)
		m_soft_iflag->set();
}

} // namespace hawk
