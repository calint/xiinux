#pragma once
#include"args.hpp"
#include"sock.hpp"
#include<netinet/tcp.h>
#include"defines.hpp"
namespace xiinux{class server final{
	inline static bool thdwatch_on{false};
	inline static pthread_t thdwatch;
	inline static void*thdwatch_run(void*arg){
		if(arg)puts((const char*)arg);
		sts.printhdr(stdout);
		while(thdwatch_on){
			int n=10;
			while(thdwatch_on and n--){
				const int sleep_us=100'000;
				usleep(sleep_us);
				sts.ms+=sleep_us/1'000;//? not really
				sts.print(stdout);
			}
			fprintf(stdout,"\n");
		}
		return nullptr;
	}
public:
	inline static void stop(){
		delete homepage;
		thdwatch_on=false;
		pthread_join(thdwatch,nullptr);
	}
	inline static int start(const int argc,const char**argv){
		args a(argc,argv);
		thdwatch_on=a.hasoption('v');
		const int port=atoi(a.getoptionvalue('p',"8088"));
		const bool option_benchmark_mode=a.hasoption('b');
		conf::print_traffic=a.hasoption('t');
		printf("%s on port %d\n",application_name,port);

		char buf[4*K];
		snprintf(buf,sizeof buf,"HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n%s\n",strlen(application_name)+1,application_name); // +1 because of '\n' after 'application_name'
		homepage=new doc(buf);

		struct sockaddr_in sa;
		const ssize_t sasz=sizeof(sa);
		bzero(&sa,sasz);
		sa.sin_family=AF_INET;
		sa.sin_addr.s_addr=INADDR_ANY;
		sa.sin_port=htons(port);
		if((srv.fd=socket(AF_INET,SOCK_STREAM,0))==-1){perror("socket");exit(1);}
		if(bind(srv.fd,(struct sockaddr*)&sa,sasz)){perror("bind");exit(2);}
		if(listen(srv.fd,nclients)==-1){perror("listen");exit(3);}
		epollfd=epoll_create(nclients);
		if(!epollfd){perror("epollcreate");exit(4);}
		struct epoll_event ev;
		ev.events=EPOLLIN;
		ev.data.ptr=&srv;
		if(epoll_ctl(epollfd,EPOLL_CTL_ADD,srv.fd,&ev)<0){perror("epolladd");exit(5);}
		struct epoll_event events[nclients];
		if(thdwatch_on)if(pthread_create(&thdwatch,nullptr,&thdwatch_run,nullptr)){perror("threadcreate");exit(6);}
		while(true){
			const int nn=epoll_wait(epollfd,events,nclients,-1);
//				if(nn==0){
//					perr("epoll 0");
//					continue;
//				}
			if(nn==-1){
				if(errno==EINTR)
					continue;// interrupted system call ok
				perr("epollwait");
				continue;
			}
			for(int i=0;i<nn;i++){
				sock*c=(sock*)events[i].data.ptr;
				if(c->fd==srv.fd){// new connection
					sts.accepts++;
					const int fda=accept(srv.fd,nullptr,nullptr);
					if(fda==-1){
						perr("accept");
						continue;
					}
					int opts=fcntl(fda,F_GETFL);
					if(opts<0){
						perr("optget");
						continue;
					}
					opts|=O_NONBLOCK;
					if(fcntl(fda,F_SETFL,opts)){
						perr("optsetNONBLOCK");
						continue;
					}
					ev.data.ptr=new sock(fda);
					ev.events=EPOLLIN|EPOLLRDHUP|EPOLLET;
	//				ev.events=EPOLLIN|EPOLLRDHUP;
	//				ev.events=EPOLLIN;
					if(epoll_ctl(epollfd,EPOLL_CTL_ADD,fda,&ev)){
						perror("epolladd");
						puts("epolladd");
						continue;
					}
					if(option_benchmark_mode){
						int flag=1;
						if(setsockopt(fda,IPPROTO_TCP,TCP_NODELAY,(void*)&flag,sizeof(int))<0){//? for performance tests
							perror("optsetTCP_NODELAY");
							puts("optsetTCP_NODELAY");
							return 8;
						}
					}
					continue;
				}
				// read or write available
				try{
					c->run();
				}catch(const char*msg){
					// todo: print session id
					delete c;
					if(msg==signal_connection_reset_by_peer){
						sts.brkp++;
						continue;
					}
//					printf(" *** exception from %p : %s\n",(void*)c,msg);
					printf(" *** exception: %s\n",msg);
				}catch(...){
					printf(" *** exception from %p\n",(void*)c);
				}
			}
		}
	}
};}
