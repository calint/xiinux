#include"web.hpp"
#include"xiinux.hpp"
#include"xiinux/sock.hpp"
static void sigexit(int i){
	puts("exiting");
	delete xiinux::homepage;
	xiinux::server_socket.close();
	signal(SIGINT,SIG_DFL);
	kill(getpid(),SIGINT);
}
int main(const int c,const char**a){
	signal(SIGINT,sigexit);
	return xiinux::main(c,a);
}
