#ifndef HAWK_DIR_CACHE_H
#define HAWK_DIR_CACHE_H

#include <unordered_map>
#include <memory>
#include <boost/filesystem/path.hpp>
#include "No_hash.h"

namespace hawk
{
	struct Dir_entry
	{
		std::time_t timestamp;
		boost::filesystem::path path;
		boost::filesystem::file_status status;
	};

	using Dir_vector = std::vector<Dir_entry>;
	using Dir_cursor = Dir_vector::iterator;
	using Dir_const_cursor = Dir_vector::const_iterator;

	class Dir_ptr
	{
	private:
		using Map = std::unordered_map<size_t,
						std::shared_ptr<Dir_vector>, No_hash>;
		using Ptr = std::shared_ptr<Dir_vector>;

		Ptr m_ptr;
		Map* m_source;
		size_t m_hash;

	public:
		Dir_ptr() : m_source{nullptr} {}
		Dir_ptr(Ptr& ptr, Map* src, size_t path_hash)
			:
			  m_ptr{ptr},
			  m_source{src},
			  m_hash{path_hash}
		{}

		~Dir_ptr();

		Dir_ptr(Dir_ptr&& ptr) noexcept;
		Dir_ptr& operator=(Dir_ptr&& ptr) noexcept;

		Dir_vector* get() const { return m_ptr.get(); }
		Dir_vector* operator->() const { return m_ptr.get(); }
		Dir_vector& operator*() const { return *m_ptr; }
	};

	Dir_ptr get_dir_ptr(const boost::filesystem::path& p, size_t path_hash);
}

#endif // HAWK_DIR_CACHE_H
