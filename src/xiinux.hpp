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
	static doc*homepage;
	static widget*widgetget(const char*qs);
	static sessions sess;
	static int epollfd;

	static void*thdwatchrun(void*arg){
		if(arg)
			puts((const char*)arg);
		sts.printhdr(stdout);
		while(1){
			int n=10;
			while(n--){
				const int sleep=100000;
				usleep(sleep);
				sts.ms+=sleep/1000;//? not really
				sts.print(stdout);
			}
			fprintf(stdout,"\n");
		}
		return nullptr;
	}
}
#endif
