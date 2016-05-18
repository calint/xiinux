#pragma once
#include"args.hpp"
#include"sock.hpp"
#include<netinet/tcp.h>
namespace xiinux{
	class server final{
		static sock sck;
		inline static void*thdwatchrun(void*arg){
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
	public:
		inline static void stop(){sck.close();delete homepage;}
		inline static int start(const int argc,const char**argv){
			args a(argc,argv);
			const bool watch_thread=a.hasoption('v');
			const int port=atoi(a.getoptionvalue('p',"8088"));
			const bool option_benchmark_mode=a.hasoption('b');
			printf("%s on port %d\n",application_name,port);

			char buf[4*K];
			// Connection: Keep-Alive for apachebench
			snprintf(buf,sizeof buf,"HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n%s",strlen(application_name),application_name);
			homepage=new doc(buf);

			struct sockaddr_in srv;
			const ssize_t srvsz=sizeof(srv);
			bzero(&srv,srvsz);
			srv.sin_family=AF_INET;
			srv.sin_addr.s_addr=INADDR_ANY;
			srv.sin_port=htons(port);
			if((sck.fd=socket(AF_INET,SOCK_STREAM,0))==-1){perror("socket");exit(1);}
			if(bind(sck.fd,(struct sockaddr*)&srv,srvsz)){perror("bind");exit(2);}
			if(listen(sck.fd,nclients)==-1){perror("listen");exit(3);}
			epollfd=epoll_create(nclients);
			if(!epollfd){perror("epollcreate");exit(4);}
			struct epoll_event ev;
			ev.events=EPOLLIN;
			ev.data.ptr=&sck;
			if(epoll_ctl(epollfd,EPOLL_CTL_ADD,sck.fd,&ev)<0){perror("epolladd");exit(5);}
			struct epoll_event events[nclients];
			pthread_t thdwatch;
			if(watch_thread)if(pthread_create(&thdwatch,nullptr,&thdwatchrun,nullptr)){perror("threadcreate");exit(6);}
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
					if(c->fd==sck.fd){// new connection
						sts.accepts++;
						const int fda=accept(sck.fd,0,0);
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
					try{
						c->run();
					}catch(const char*msg){
						if(!strcmp(msg,"brk")){
	//					if(msg!=exception_connection_reset_by_client){
							printf(" *** exception from %p : %s\n",(void*)c,msg);
						}
						delete c;
					}catch(...){
						printf(" *** exception from %p\n",(void*)c);
						delete c;
					}
				}
			}
		}
	};
	sock server::sck{0};
}
