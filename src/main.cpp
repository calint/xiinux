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
	static int const port=8088;

	static doc*homepage;

	static widget*widgetget(const char*qs);

	static inline char*strtrm(char*p,char*e){
		while(p!=e&&isspace(*p))
			p++;
		while(p!=e&&isspace(*e))
			*e--=0;
		return p;
	}
	static inline void strlwr(char*p){
		while(*p){
			*p=(char)tolower(*p);
			p++;
		}
	}

	static sessions sess;
//	static widget*widgetget(const char*qs);
	static int epollfd;
	static inline void urldecode(char*str){
		const char*p=str;
		while(*p){
			char a,b;
			if(*p=='%'&&(a=p[1])&&(b=p[2])&&isxdigit(a)&&isxdigit(b)){
				if(a>='a')a-=(char)('a'-'A');//?
				if(a>='A')a-='A'-10;else a-='0';
				if(b>='a')b-='a'-'A';//?
				if(b>='A')b-='A'-10;else b-='0';
				*str++=16*a+b;
				p+=3;
				continue;
			}
			*str++=*p++;
		}
		*str++='\0';
	}
	#define perr(str) printf("\n\n%s:%d ",__FILE__,__LINE__);perror(str);
//	#define dbg(str) printf("%s:%d %s   %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,str);
	#define dbg(str)
	class sock{
		enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,read_content,upload,next_request};
//		enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,read_content,upload};
		parser_state state{next_request};
		int file_fd{0};
		off_t file_pos{0};
		size_t file_len{0};
		int upload_fd{0};
		char*pth{nullptr};
		char*qs{nullptr};
		lut<const char*>hdrs;
		char*hdrk{nullptr};
		char*hdrv{nullptr};
		char*content{nullptr};
		size_t content_len{0};
		size_t content_pos{0};
		char buf[conbufnn];
		char*bufp{buf};
		size_t bufi{0};
		size_t bufnn{0};
	//	void reset_for_new_request(){
	//		file_fd=0;
	//		file_pos=0;
	//		file_len=0;
	//		upload_fd=0;
	//		pth=nullptr;
	//		qs=nullptr;
	//		hdrs.clear();
	//		hdrk=nullptr;
	//		hdrv=nullptr;
	//		content=nullptr;
	//		content_len=0;
	//		content_pos=0;
	//		//? clear buf
	//		bufp=buf;
	//		bufi=0;
	//		bufnn=0;
	//	}
		inline void io_request_read(){
			struct epoll_event ev;
			ev.data.ptr=this;
			ev.events=EPOLLIN;
			if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)){
				perror("ioreqread");
				throw"epollmodread";
			}
		}
		inline void io_request_write(){
			struct epoll_event ev;
			ev.data.ptr=this;
			ev.events=EPOLLOUT;
			if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)){
				perror("ioreqwrite");
				throw"epollmodwrite";
			}
		}
	public:
		int fd{0};
		inline sock(const int f):fd(f){
			sts.socks++;
//			printf("%s:%d %s : new %d\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,fd);
		}
		inline~sock(){
			sts.socks--;
//			printf("%s:%d %s : delete %d\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,fd);
			//printf(" * delete sock %p\n",(void*)this);
			delete[]content;
			if(!::close(fd)){
				return;
			}
			sts.errors++;
	//		printf("%s:%d ",__FILE__,__LINE__);perror("sockdel");
			printf("%s:%d %s : sockdel\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);
			perror("sockdel");
		}
		inline void close(){
			::close(fd);
		}
		void run(){while(true){
			//printf(" state %d\n",state);
			if(state==upload){
				char upload_buffer[4*K];
				const size_t upload_remaining=content_len-content_pos;
				sts.reads++;
				const ssize_t nn=recv(fd,upload_buffer,upload_remaining>sizeof upload_buffer?sizeof upload_buffer:upload_remaining,0);
				if(nn==0){//closed by client
					throw"brk";
				}
				if(nn<0){
					if(errno==EAGAIN||errno==EWOULDBLOCK){
	//					printf("eagain || wouldblock\n");
						io_request_read();
						return;
					}else if(errno==ECONNRESET){
						throw"brk";
					}
	//				printf("\n\n%s:%d ",__FILE__,__LINE__);
	//				perror("upload");
					perr("upload");
					sts.errors++;
					throw"upload";
				}
				const ssize_t nw=write(upload_fd,upload_buffer,(size_t)nn);
				if(nw<0){
					printf("\n\n%s:%d ",__FILE__,__LINE__);
					perror("writing upload to file");
					sts.errors++;
					throw"writing upload to file";
				}
				if(nw!=nn){
					throw"writing upload to file 2";
				}
				sts.input+=(size_t)nn;
				content_pos+=(size_t)nn;
	//			printf(" uploading %s   %zu of %zu\n",pth+1,content_pos,content_len);
				if(content_pos<content_len){
					continue;
				}
				if(::close(upload_fd)<0){
					perror("while closing file");
				}
				reply::io_send(fd,"HTTP/1.1 204\r\n\r\n",16,true);
	//			printf("      done %s\n",pth+1);
				state=next_request;
			}else if(state==read_content){
				sts.reads++;
				const ssize_t nn=recv(fd,content+content_pos,content_len-content_pos,0);
				if(nn==0){//closed by client
					delete this;
					return;
				}
				if(nn<0){
					if(errno==EAGAIN||errno==EWOULDBLOCK){
	//					printf("eagain || wouldblock\n");
						io_request_read();
						return;
					}else if(errno==ECONNRESET){
						throw"brk";
					}
					printf("\n\n%s:%d ",__FILE__,__LINE__);perror("readcontent");
					sts.errors++;
					throw"readingcontent";
				}
				content_pos+=(unsigned)nn;
				sts.input+=(unsigned)nn;
				if(content_pos==content_len){
					*(content+content_len)=0;
					process();
					break;
				}
				return;
			}else if(state==resume_send_file){
				sts.writes++;
				const ssize_t sf=sendfile(fd,file_fd,&file_pos,file_len);
				if(sf<0){//error
					if(errno==EAGAIN){
						io_request_write();
						return;
					}
					sts.errors++;
					printf("\n\n%s:%d ",__FILE__,__LINE__);perror("resumesendfile");
					delete this;
					return;
				}
				file_len-=size_t(sf);
				sts.output+=size_t(sf);
				if(file_len!=0){
	//				state=resume_send_file;
					io_request_write();
					return;
				}
				::close(file_fd);
				state=next_request;
			}
			if(bufi==bufnn){//? assumes request and headers fit in conbufnn and done in one read
				if(bufi>=conbufnn)
					throw"reqbufoverrun";//? chained requests buf pointers
				if(state==next_request){//? next_request
					bufi=bufnn=0;
					bufp=buf;
				}
				sts.reads++;
				const ssize_t nn=recv(fd,bufp,conbufnn-bufi,0);
				if(nn==0){//closed by client
					delete this;
					return;
				}
				if(nn<0){//error
					if(errno==EAGAIN||errno==EWOULDBLOCK){
						io_request_read();
						return;
					}else if(errno==ECONNRESET){
						delete this;
						return;
					}
//					printf("\n\n%s:%d ",__FILE__,__LINE__);perror("readbuf");
					sts.errors++;
					delete this;
					return;
				}
				bufnn+=(size_t)nn;
//				printf("%s : %s",__PRETTY_FUNCTION__,buf);fflush(stdout);
				sts.input+=(unsigned)nn;
			}
			if(state==next_request){
				sts.requests++;
				state=method;
			}
			if(state==method){
				while(bufi<bufnn){
					bufi++;
					const char c=*bufp++;
					if(c==' '){
						state=uri;
						pth=bufp;
						qs=nullptr;
						break;
					}
				}
			}
			if(state==uri){
				while(bufi<bufnn){
					bufi++;
					const char c=*bufp++;
					if(c==' '){
						state=protocol;
						*(bufp-1)=0;
						urldecode(pth);
						break;
					}else if(c=='?'){
						state=query;
						qs=bufp;
						*(bufp-1)=0;
						break;
					}
				}
			}
			if(state==query){
				while(bufi<bufnn){
					bufi++;
					const char c=*bufp++;
					if(c==' '){
						state=protocol;
						*(bufp-1)=0;
						urldecode(qs);
						break;
					}
				}
			}
			if(state==protocol){
				while(bufi<bufnn){
					bufi++;
					const char c=*bufp++;
					if(c=='\n'){
						hdrs.clear();
						hdrk=bufp;
						state=header_key;
						break;
					}
				}
			}
			if(state==header_key){
	read_header_key:
				while(bufi<bufnn){
					bufi++;
					const char c=*bufp++;
					if(c=='\n'){// content or done parsing
						const char*content_length_str=hdrs["content-length"];
						if(!content_length_str){
							content=nullptr;
							content_len=0;//? already
							process();
							break;
						}
						content_len=(size_t)atoll(content_length_str);
						const char*content_type=hdrs["content-type"];
						if(content_type&&strstr(content_type,"file")){// file upload
	//							printf("uploading file: %s   size: %s\n",pth+1,content_length_str);
							const mode_t mod{0664};
							char buf[255];
							snprintf(buf,sizeof buf,"upload/%s",pth+1);
							upload_fd=open(buf,O_CREAT|O_WRONLY|O_TRUNC,mod);
							if(upload_fd<0){
								perror("while creating file for upload");
								throw"err";
							}
							const char*s=hdrs["expect"];
							if(s&&!strcmp(s,"100-continue")){
	//								printf("client expects 100 continue before sending post\n");
								reply::io_send(fd,"HTTP/1.1 100\r\n\r\n",16,true);
								state=upload;
								break;
							}
							const size_t chars_left_in_buffer=bufnn-bufi;
							if(chars_left_in_buffer==0){
								state=upload;
								break;
							}
							if(chars_left_in_buffer>=content_len){
								dbg("upload fits in buffer");
								const ssize_t nn=write(upload_fd,bufp,(size_t)content_len);
								if(nn<0){
									perror("while writing upload to file");
									throw"err";
								}
								sts.input+=(size_t)nn;
								if((size_t)nn!=content_len){
									throw"incomplete upload";
								}
								if(::close(upload_fd)<0){
									perror("while closing file");
								}
	//							const char resp[]="HTTP/1.1 200\r\nContent-Length: 0\r\n\r\n";
								const char resp[]="HTTP/1.1 204\r\n\r\n";
								reply::io_send(fd,resp,sizeof resp-1,true);//. -1 to remove eos
								bufp+=content_len;
								bufi+=content_len;
								state=next_request;
								break;
							}
							const ssize_t nn=write(fd,bufp,chars_left_in_buffer);
							if(nn<0){
								perror("while writing upload to file2");
								throw"err";
							}
							content_pos=(size_t)nn;
							state=upload;
							break;
						}
						// posted content
						delete[]content;
						content=new char[content_len+1];// extra char for end-of-string
						const size_t chars_left_in_buffer=bufnn-bufi;
						if(chars_left_in_buffer>=content_len){
							memcpy(content,bufp,(size_t)content_len);
							*(content+content_len)=0;
							bufp+=content_len;
							bufi+=content_len;
							process();
							break;
						}else{
							memcpy(content,bufp,chars_left_in_buffer);
							content_pos=chars_left_in_buffer;
							const char*s=hdrs["expect"];
							if(s&&!strcmp(s,"100-continue")){
								reply::io_send(fd,"HTTP/1.1 100\r\n\r\n",16,true);
							}
							state=read_content;
							break;
						}
					}else if(c==':'){
						*(bufp-1)=0;
						hdrv=bufp;
						state=header_value;
						break;
					}
				}
			}
			if(state==header_value){
				while(bufi<bufnn){
					bufi++;
					const char c=*bufp++;
					if(c=='\n'){
						*(bufp-1)=0;
						hdrk=strtrm(hdrk,hdrv-2);
						strlwr(hdrk);
						hdrv=strtrm(hdrv,bufp-2);
						hdrs.put(hdrk,hdrv);
						hdrk=bufp;
						state=header_key;
	//					break;
						goto read_header_key;
					}
				}
			}
		}}
	private:
		void process(){
			const char*path=*pth=='/'?pth+1:pth;
			reply x=reply(fd);
			if(!*path&&qs){
				sts.widgets++;
				const char*cookie=hdrs["cookie"];
				const char*session_id;
				if(cookie&&strstr(cookie,"i=")){//? parse cookie
					session_id=cookie+sizeof "i="-1;
				}else{
					session_id=nullptr;
				}
				session*ses;
				if(!session_id){
					// create session
					//"Fri, 31 Dec 1999 23:59:59 GMT"
					time_t timer=time(NULL);
					struct tm*tm_info=gmtime(&timer);
					char*sid=(char*)malloc(24);
					//						 20150411--225519-ieu44d
					strftime(sid,size_t(24),"%Y%m%d-%H%M%S-",tm_info);
					char*sid_ptr=sid+16;
					for(int i=0;i<7;i++){
						*sid_ptr++='a'+(char)(random()%26);
					}
					*sid_ptr=0;
	//				printf(" * creating session %s\n",session_id);
					ses=new session(sid);
					sess.put(sid,ses,false);
//					sess.all.put(sid,ses,false);
					x.send_session_id_at_next_opportunity(sid);
				}else{
					ses=sess.get_session(session_id);
					if(!ses){// session not found, reload
	//					printf(" * session not found, recreating: %s\n",session_id);
						char*sid=(char*)malloc(64);
	//					if(strlen(session_id)>23)throw"cookielen";
						strncpy(sid,session_id,64);
		//				printf(" * creating session %s\n",session_id);
						ses=new session(sid);
//						sess.all.put(sid,ses,false);
						sess.put(sid,ses,false);
					}
				}
				widget*o=ses->get_widget(qs);
				if(!o){
	//				printf(" * widget not found in session, creating  %s\n",qs);
					o=widgetget(qs);
					const size_t key_len=strlen(qs);
					char*key=(char*)malloc(key_len+1);
					memcpy(key,qs,key_len+1);
					ses->put_widget(key,o);
				}
				if(content){
	//				printf(" * content:\n%s\n",content);
					o->on_content(x,content,content_len);
					delete[]content;
					content=nullptr;
					state=next_request;
					return;
				}else{
					o->to(x);
					state=next_request;
					return;
				}
			}
			if(!*path){
				sts.cache++;
				homepage->to(x);
				state=next_request;
				return;
			}
			if(strstr(path,"..")){
				x.http2(403,"path contains ..");
				state=next_request;
				return;
			}
			sts.files++;
			struct stat fdstat;
			if(stat(path,&fdstat)){
				x.http2(404,"not found");
				state=next_request;
				return;
			}
			if(S_ISDIR(fdstat.st_mode)){
				x.http2(403,"path is directory");
				state=next_request;
				return;
			}
			const struct tm*tm=gmtime(&fdstat.st_mtime);
			char lastmod[64];
			//"Fri, 31 Dec 1999 23:59:59 GMT"
			strftime(lastmod,sizeof lastmod,"%a, %d %b %y %H:%M:%S %Z",tm);
			const char*lastmodstr=hdrs["if-modified-since"];
			if(lastmodstr&&!strcmp(lastmodstr,lastmod)){
				const char hdr[]="HTTP/1.1 304\r\n\r\n";
				const size_t hdrnn=sizeof hdr;
				reply::io_send(fd,hdr,hdrnn,true);
				state=next_request;
				return;
			}
			file_fd=open(path,O_RDONLY);
			if(file_fd==-1){
				x.http2(404,"cannot open");
				state=next_request;
				return;
			}
			file_pos=0;
			file_len=size_t(fdstat.st_size);
			const char*range=hdrs["range"];
			char bb[K];
			int bb_len;
			if(range&&*range){
				off_t rs=0;
				if(EOF==sscanf(range,"bytes=%jd",&rs)){
					sts.errors++;
//					printf("\n\n%s:%d ",__FILE__,__LINE__);perror("scanrange");
					throw"errrorscanning";
				}
				file_pos=rs;
				const size_t e=file_len;
				file_len-=(size_t)rs;
	//			bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 206\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",lastmod,file_len,rs,e,e);
				bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 206\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",lastmod,file_len,rs,e,e);
			}else{
				// Connection: Keep-Alive\r\n for apache-bench
	//			bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 200\r\nConnection: Keep-Alive\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\n\r\n",lastmod,file_len);
				bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 200\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\n\r\n",lastmod,file_len);
			}
			if(bb_len<0)throw"err";
			reply::io_send(fd,bb,(size_t)bb_len,true);
			const ssize_t nn=sendfile(fd,file_fd,&file_pos,file_len);
			if(nn<0){
				if(errno==EPIPE||errno==ECONNRESET){
					sts.brkp++;
					throw"brk";
				}
				sts.errors++;
//				printf("\n\n%s:%d ",__FILE__,__LINE__);perror("sendfile");
				throw"err";
			}
			sts.output+=size_t(nn);
			file_len-=size_t(nn);
			if(file_len!=0){
				state=resume_send_file;
				io_request_write();
				return;
			}
			::close(file_fd);
			state=next_request;
		}
	};
	static sock server_socket(0);
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
	static void sigexit(int i){
		puts("exiting");
		delete homepage;
	//	if(shutdown(server_socket.fd,SHUT_RDWR))perror("shutdown");
		server_socket.close();
	//	close(server_socket.fd);
		signal(SIGINT,SIG_DFL);
		kill(getpid(),SIGINT);
	//	exit(i);
	}
	//bool func(const char*li){
	//	puts(li);
	//	return true;
	//};
	//void test_xprinter(xprinter&x){
	//	x.p("hello world from test xprinter").nl();
	//}
	int main(int argc,char**argv){
	//	lst<const char*>ls;
	//	ls.to(stdout);
	////	const char*cc_hello="hello";
	//	ls.add("hello");
	//	ls.to(stdout);
	//	ls.addfirst("world");
	//	ls.to(stdout);
	//	const char*s=ls.take_first();
	//	ls.to(stdout);
	//	ls.add(s);
	//	ls.foreach(func);
	//	ls.foreach2([](const char*li){puts(li);return true;});
	//	const char*hello=ls.find("hello");
	//	ls.clear();
	//	ls.to(stdout);
	//	return 0;
	//
	//	void*b1=malloc(256);
	//	void*b3=0;
	////	void*b3=malloc(128);
	//	void*b2=realloc(b1,512);
	//	printf("%p  %p    %p   \n",b1,b2,b3);
	//	return 0;
	//
	//	strb sb("hello there");
	//	test_xprinter(sb);
	//	sb.to(stdout);
	//	return 0;

		signal(SIGINT,sigexit);
		printf("%s on port %d\n",APP,port);

		char buf[4*K];
		// Connection: Keep-Alive for apachebench
	//	snprintf(buf,sizeof buf,"HTTP/1.1 200\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\n\r\n%s",strlen(APP),APP);
		snprintf(buf,sizeof buf,"HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n%s",strlen(APP),APP);
		homepage=new doc(buf);

		struct sockaddr_in srv;
		const ssize_t srvsz=sizeof(srv);
		bzero(&srv,srvsz);
		srv.sin_family=AF_INET;
		srv.sin_addr.s_addr=INADDR_ANY;
		srv.sin_port=htons(port);
		if((server_socket.fd=socket(AF_INET,SOCK_STREAM,0))==-1){perror("socket");exit(1);}
		if(bind(server_socket.fd,(struct sockaddr*)&srv,srvsz)){perror("bind");exit(2);}
		if(listen(server_socket.fd,nclients)==-1){perror("listen");exit(3);}
		epollfd=epoll_create(nclients);
		if(!epollfd){perror("epollcreate");exit(4);}
		struct epoll_event ev;
		ev.events=EPOLLIN;
		ev.data.ptr=&server_socket;
		if(epoll_ctl(epollfd,EPOLL_CTL_ADD,server_socket.fd,&ev)<0){perror("epolladd");exit(5);}
		struct epoll_event events[nclients];
		const bool watch_thread=argc>1;
		pthread_t thdwatch;
		if(watch_thread)if(pthread_create(&thdwatch,nullptr,&thdwatchrun,nullptr)){perror("threadcreate");exit(6);}
		while(true){
			//printf(" epoll_wait\n");
			const int nn=epoll_wait(epollfd,events,nclients,-1);
			//printf(" epoll_wait returned %d\n",nn);
			if(nn==0){
				puts("epoll 0");
				continue;
			}
			if(nn==-1){
				perror("epollwait");
				continue;
			}
			for(int i=0;i<nn;i++){
				sock*c=(sock*)events[i].data.ptr;
				if(c->fd==server_socket.fd){// new connection
					sts.accepts++;
					const int fda=accept(server_socket.fd,0,0);
					if(fda==-1){
						perror("accept");
						puts("accept");
						continue;
	//					exit(8);
					}
					int opts=fcntl(fda,F_GETFL);
					if(opts<0){
						perror("optget");
						puts("optget");
						continue;
	//					exit(9);
					}
					opts|=O_NONBLOCK;
					if(fcntl(fda,F_SETFL,opts)){
						perror("optsetNONBLOCK");
						puts("optsetNONBLOCK");
						continue;
	//					exit(10);
					}
					ev.data.ptr=new sock(fda);
					ev.events=EPOLLIN|EPOLLRDHUP|EPOLLET;
	//				ev.events=EPOLLIN|EPOLLRDHUP;
	//				ev.events=EPOLLIN;
					if(epoll_ctl(epollfd,EPOLL_CTL_ADD,fda,&ev)){
						perror("epolladd");
						puts("epolladd");
						continue;
	//					exit(11);
					}
					int flag=1;
					if(setsockopt(fda,IPPROTO_TCP,TCP_NODELAY,(void*)&flag,sizeof(int))<0){//? for performance tests
						perror("optsetTCP_NODELAY");
						puts("optsetTCP_NODELAY");
						exit(12);
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
}

#include"web.hpp"

int main(int c,char**a){
	return xiinux::main(c,a);
}
