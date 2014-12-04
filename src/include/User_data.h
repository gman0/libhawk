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

#ifndef HAWK_USER_DATA_H
#define HAWK_USER_DATA_H

#include <typeinfo>
#include <utility>
#include <memory>
#include <cassert>
#include "Not_self_type.h"

namespace hawk {

	// This class is similar to boost::any, used in Dir_entry.
	class User_data
	{
	public:
		User_data() : m_content{nullptr} {}

		User_data(User_data&&) = default;
		User_data& operator=(User_data&&) = default;

		// User can supply any type he wants but it has to stay
		// the same throughout the whole application life-time,
		// otherwise an assert will fire.
		// The supplied type is required to be CopyConstructible
		// and/or MoveConstructible, otherwise a static_assert
		// will fire.
		template <typename T, enable_if_not_self<T, User_data>* = nullptr>
		User_data(T&& value)
			: m_content{new Holder<typename std::decay<T>::type>{
					std::forward<T>(value)}}
		{}

		User_data(const User_data& ud) = delete;

		template <typename T, enable_if_not_self<T, User_data>* = nullptr>
		User_data& operator=(T&& value)
		{
			User_data tmp = User_data {std::forward<T>(value)};
			m_content = tmp.m_content;

			return *this;
		}

		bool empty() const;
		void clear();
		const std::type_info& type() const;

	private:
		class Placeholder
		{
		public:
			virtual ~Placeholder() = default;
			virtual const std::type_info& type() const noexcept = 0;
		};

		template <typename T>
		class Holder : public Placeholder
		{
		public:
			template <typename T_>
			Holder(T_&& value)
				: m_held{std::forward<T_>(value)}
			{
				static_assert(std::is_same<
								typename std::decay<T_>::type, T>::value,
							  "Type supplied to ctor of User_data::Holder is "
							  "required to be the same as type supplied to "
							  "User_data::Holder<>");

				static_assert(std::is_copy_constructible<T>::value ||
							  std::is_move_constructible<T>::value,
							  "Type supplied to User_data needs to be "
							  "CopyConstructible and/or MoveConstructible");

				static const std::type_info& info = typeid(T);
				assert(info == typeid(T) && "User_data can store only data of "
					   "the same type");
			}

			virtual const std::type_info& type() const noexcept
			{
				return typeid(T);
			}

		public:
			T m_held;
		};

		std::unique_ptr<Placeholder> m_content;

		template <typename T>
		friend T* user_data_cast(User_data*) noexcept;
	};

	template <typename T>
	T* user_data_cast(User_data* ud) noexcept
	{
		if (ud->type() != typeid(T))
			return nullptr;

		User_data::Holder<T>* val =
				static_cast<User_data::Holder<T>*>(ud->m_content.get());

		return &val->m_held;
	}

} // namespace hawk

#endif // HAWK_USER_DATA_H
