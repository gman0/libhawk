#include <boost/filesystem.hpp>
#include "Tab.h"

using namespace hawk;
using namespace boost::filesystem;

const path& Tab::get_pwd() const
{
	return m_pwd;
}

void Tab::set_pwd(const path& pwd)
{
	m_pwd = pwd;
}
