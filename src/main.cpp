#include"xiinux.hpp"
#include"web.hpp"
#include"sock.hpp"
static void sigexit(int i){
	puts("exiting");
	delete xiinux::homepage;
	xiinux::server_socket.close();
	signal(SIGINT,SIG_DFL);
	kill(getpid(),SIGINT);
}
int main(int c,char**a){
	signal(SIGINT,sigexit);
	return xiinux::main(c,a);
}
