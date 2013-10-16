#include <functional>
#include "Handler.h"

using namespace std;
using namespace hawk;
using namespace boost::filesystem;

Handler::Handler(const path& p, const string& type)
	:
	m_path{p}
{
	m_type = hash<string>()(type);
}

size_t Handler::get_type() const
{
	return m_type;
}

bool Handler::operator==(const Handler& h)
{
	return (m_type == h.m_type);
}

bool Handler::operator!=(const Handler& h)
{
	return (m_type != h.m_type);
}
