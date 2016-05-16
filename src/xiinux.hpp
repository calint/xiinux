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
#include"lst.hpp"
#include"lut.hpp"
#include"stats.hpp"
#include"xprinter.hpp"
#include"strb.hpp"
#include"doc.hpp"
#include"reply.hpp"
#include"session.hpp"
#include"sessions.hpp"
namespace xiinux{
	#define APP "xiinux web server"
	static int const K=1024;
	static size_t const conbufnn=K;
	static int const nclients=K;
//	static int const port=8088;
	static doc*homepage;
	static widget*widgetget(const char*qs);
	static sessions sess;
	static int epollfd;


	#define perr(str) printf("\n\n%s:%d ",__FILE__,__LINE__);perror(str);
//	#define dbg(str) printf("%s:%d %s   %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,str);
	#define dbg(str)
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
	//bool func(const char*li){
	//	puts(li);
	//	return true;
	//};
	//void test_xprinter(xprinter&x){
	//	x.p("hello world from test xprinter").nl();
	//}
}
#endif
