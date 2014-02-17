/*
	Copyright (C) 2013 Róbert "gman" Vašek <gman@codefreax.org>

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

#ifndef HAWK_COLUMN_H
#define HAWK_COLUMN_H

#include <memory>
#include <utility>
#include <vector>
#include "Handler.h"
#include "handlers/dir.h"

namespace hawk {
	class Tab;

	class Column
	{
	private:
		using Type_product =
			std::function<Handler*(const boost::filesystem::path&,
									Column*)>;

		boost::filesystem::path m_path;

		std::shared_ptr<Handler> m_handler;
		Type_product m_handler_closure;

		const Column* m_child_column;
		Tab* m_parent_tab;

	public:
		Column()
			:
			m_child_column{},
			m_parent_tab{}
		{}

		Column(const boost::filesystem::path& path,
			const Type_product& tp, Tab* parent_tab);
		Column(boost::filesystem::path&& path,
			const Type_product& tp, Tab* parent_tab);

		// for internal/expert use only
		void _ready();
		void _set_child_column(const Column* child_column);
		void _set_parent_tab(Tab* tab);

		Handler* get_handler();
		const Handler* get_handler() const;

		const boost::filesystem::path& get_path() const;
		void set_path(const boost::filesystem::path& path);
		void set_path(boost::filesystem::path&& path);

		const boost::filesystem::path* get_child_path() const;
		Tab* get_parent_tab() { return m_parent_tab; }
	};
}

#endif // HAWK_COLUMN_H
