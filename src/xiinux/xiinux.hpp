#pragma once
#include"doc.hpp"
#include"sessions.hpp"
#include"widget.hpp"
namespace xiinux{// shared by server and sock to avoid circular ref
	static int epollfd;
	static doc*homepage;
	static widget*widgetget(const char*qs);
}
