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

#ifndef HAWK_USER_DATA_H
#define HAWK_USER_DATA_H

#include <type_traits>
#include <typeinfo>
#include <utility>
#include <cassert>

namespace hawk {

	// This class is similar to boost::any, used in Dir_entry.
	class User_data
	{
	public:
		User_data() : m_content{nullptr} {}
		~User_data();

		template <typename T>
		User_data(T&& value)
			: m_content{new Holder<typename std::decay<T>::type>{
					std::forward<T>(value)}}
		{}

		User_data(const User_data& ud);
		User_data(User_data&& ud) noexcept;

		User_data& swap(User_data& ud) noexcept;
		User_data& operator=(const User_data& ud);
		User_data& operator=(User_data&& rhs) noexcept;

		template <typename T>
		User_data& operator=(T&& value)
		{
			User_data(std::forward<T>(value)).swap(*this);
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

		public:
			virtual const std::type_info& type() const noexcept = 0;
			virtual Placeholder* clone() const = 0;
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
				static const std::type_info& info = typeid(T);
				assert(info == typeid(T) && "User_data can store only data of "
					   "the same type");
			}

			virtual const std::type_info& type() const noexcept
			{
				return typeid(T);
			}

			virtual Placeholder* clone() const
			{
				return new Holder(m_held);
			}

		public:
			T m_held;
		};

		Placeholder* m_content;
		template <typename T>
		friend T* user_data_cast(User_data*) noexcept;
	};

	inline void swap(User_data& lhs, User_data& rhs) noexcept
	{
		lhs.swap(rhs);
	}

	template <typename T>
	T* user_data_cast(User_data* ud) noexcept
	{
		return (ud->type() == typeid(T))
				? static_cast<User_data::Holder<T>*>(ud->m_content)->m_held
				  : nullptr;
	}

} // namespace hawk

#endif // HAWK_USER_DATA_H
