#ifndef xiinux_hpp
#define xiinux_hpp
#include"xiinux/doc.hpp"
#include"xiinux/sessions.hpp"
#include"xiinux/widget.hpp"
namespace xiinux{// shared by server and sock to avoid circular ref
	static int epollfd;
	static doc*homepage;
	static widget*widgetget(const char*qs);
}
#endif
