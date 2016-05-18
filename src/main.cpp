#include"xiinux/server.hpp"
#include<signal.h>
static void sigint(int i){
	puts("exiting");
	xiinux::server::stop();
	signal(SIGINT,SIG_DFL);
	kill(getpid(),SIGINT);
}
int main(const int c,const char**a){
	signal(SIGINT,sigint);
	return xiinux::server::start(c,a);
}

