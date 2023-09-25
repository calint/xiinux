#include"xiinux/server.hpp"
#include<signal.h>
static void sigint(int i){
	puts("exiting");
	xiinux::server::stop();
	exit(0);
}
int main(const int c,const char**a){
	signal(SIGINT,sigint);
	signal(SIGPIPE,SIG_IGN);// if not ignored 'sendfile' aborts program when 'Broken pipe'
	return xiinux::server::start(c,a);
}

