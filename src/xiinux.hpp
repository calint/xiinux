#ifndef xiinux_hpp
#define xiinux_hpp
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<errno.h>
#include<signal.h>
#include<unistd.h>
#include<sys/sendfile.h>
#include<pthread.h>
//#include<thread>
#include<ctype.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<netinet/tcp.h>
#include<typeinfo>
#include<functional>
#include"xiinux/lst.hpp"
#include"xiinux/lut.hpp"
#include"xiinux/stats.hpp"
#include"xiinux/xprinter.hpp"
#include"xiinux/strb.hpp"
#include"xiinux/doc.hpp"
#include"xiinux/reply.hpp"
#include"xiinux/session.hpp"
#include"xiinux/sessions.hpp"
#include"xiinux/widget.hpp"
#include"xiinux/defines.hpp"
namespace xiinux{
	static int epollfd;
	static doc*homepage;
	static sessions sess;
	static widget*widgetget(const char*qs);
}
#endif
