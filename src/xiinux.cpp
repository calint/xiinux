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
#include<ctype.h>
#include<sys/types.h>
#include<sys/stat.h>
//#include<thread>
#include<netinet/tcp.h>
static const char*app="xiinux web server";
static int const K=1024;
static size_t const conbufnn=K;
static int const nclients=K;
static int const port=8088;
class stats{
public:
	unsigned long long int ms{0};
	unsigned long long int input{0};
	unsigned long long int output{0};
	unsigned long long int accepts{0};
	unsigned long long int reads{0};
	unsigned long long int writes{0};
	unsigned long long int files{0};
	unsigned long long int widgets{0};
	unsigned long long int cache{0};
	unsigned long long int errors{0};
	unsigned long long int brkp{0};
	void printhdr(FILE*f){
		fprintf(f,"%12s%12s%12s%8s%8s%8s%8s%8s%8s%8s%8s\n","ms","input","output","accepts","reads","writes","files","widgets","cache","errors","brkp");
		fflush(f);
	}
	void print(FILE*f){
		fprintf(f,"\r%12llu%12llu%12llu%8llu%8llu%8llu%8llu%8llu%8llu%8llu%8llu",ms,input,output,accepts,reads,writes,files,widgets,cache,errors,brkp);
		fflush(f);
	}
};
static stats stats;
class xwriter{
	int fd;
public:
	xwriter(const int fd=0):fd(fd){}
	xwriter&reply_http(const int code,const char*content){
		const size_t nn=strlen(content);
		char bb[K];
		sprintf(bb,"HTTP/1.1 %d\r\nConnection: Keep-Alive\r\nContent-Length: %lu\r\n\r\n",code,nn);
		pk(bb).pk(content,nn);
		return*this;
	}
	xwriter&pk(const char*s,const size_t nn){
		const ssize_t n=send(fd,s,nn,0);
		if(n==-1){
			if(errno==32)
				throw"broken pipe";
			perror("send");
			printf("\n\n%s  %d   errorno=%d\n\n",__FILE__,__LINE__,errno);
			throw"unknown error while sending";
		}
		stats.output+=n;
		if(n!=ssize_t(nn)){
			stats.errors++;
			printf("\n\n%s  %d    sent %lu of %lu\n\n",__FILE__,__LINE__,n,nn);
		}
		return*this;
	}
	inline xwriter&pk(const char*s){const size_t snn=strlen(s);return pk(s,snn);}
};
class doc{
	ssize_t size;
	char*buf;
	const char*lastmod;
public:
	doc(const char*data,const char*lastmod=nullptr):size(strlen(data)),buf(new char[size]),lastmod(lastmod){
		memcpy(buf,data,size);
	}
	~doc(){delete buf;}
	inline const char*getbuf()const{return buf;}
	inline ssize_t getsize()const{return size;}
	inline void to(xwriter&x)const{x.pk(buf,size);}
};
doc*homepage;
class widget{
public:
	virtual ~widget(){};
	virtual void to(xwriter&x)=0;
	virtual void ax(xwriter&x,char*a[]=0){if(a)x.pk(a[0]);}
};
static widget*widgetget(const char*qs);
static char*strtrm(char*p,char*e){
	while(p!=e&&isspace(*p))
		p++;
	while(p!=e&&isspace(*e))
		*e--=0;
	return p;
}
static void strlwr(char*p){
	while(*p){
		*p=(char)tolower(*p);
		p++;
	}
}
template<class T>class lut{
private:
	int size;
	class el{
	public:
		const char*key;
		T data;
		el*nxt;
		el(const char*key,T data):key(key),data(data),nxt(nullptr){}
		~el(){
			if(nxt)
				delete nxt;
		}
	};
	el**array;
public:
	static unsigned int hash(const char*key,const unsigned int roll){
		unsigned int i=0;
		const char*p=key;
		while(*p)
			i+=*p++;
		i%=roll;
		return i;
	}
	lut(const int size=8):size(size){
		array=(el**)calloc(size,sizeof(el*));
	}
	~lut(){
		clear();
		delete array;
	}
	T operator[](const char*key){
		const int h=hash(key,size);
		el*e=array[h];
		if(!e)
			return nullptr;
		while(true){
			if(!strcmp(e->key,key)){
				return e->data;
			}
			if(e->nxt){
				e=e->nxt;
				continue;
			}
			return nullptr;
		}
	}
	void put(const char*key,T data){
		const int h=hash(key,size);
		el*l=array[h];
		if(!l){
			array[h]=new el(key,data);
			return;
		}
		while(1){
			if(!strcmp(l->key,key)){
				l->data=data;
				return;
			}
			if(l->nxt){
				l=l->nxt;
				continue;
			}
			l->nxt=new el(key,data);
			return;
		}
	}
	void clear(){
		for(int i=0;i<size;i++){
			el*e=array[i];
			if(!e)
				continue;
			delete e;
			array[i]=nullptr;
		}
	}
};
class sock{
private:
	int state{0};
	int fdfile{0};
	off_t fdfileoffset{0};
	long long fdfilecount{0};
	char*pth{nullptr};
	char*qs{nullptr};
	lut<const char*>hdrs;
	char*hdrp{nullptr};
	char*hdrvp{nullptr};
public:
	int fd;
	char buf[conbufnn];
	char*bufp{buf};
	size_t bufi{0};
	size_t bufnn{0};
	sock(const int fd=0):fd(fd){}
	~sock(){
		if(!close(fd))return;
		stats.errors++;
		perror("delete sock");printf(" %s  %d\n\n",__FILE__,__LINE__);
	}
	int run(){
		if(state==6){
			const ssize_t sf=sendfile(fd,fdfile,&fdfileoffset,fdfilecount);
			if(sf<0)
				{stats.errors++;perror("resume sendfile");printf("\n\n%s  %d\n\n",__FILE__,__LINE__);return 0;}
			fdfilecount-=sf;
			stats.output+=sf;
			if(fdfilecount!=0){
				state=6;
				return 2;
			}
			close(fdfile);
			state=0;
		}
		while(bufi<bufnn){
			bufi++;
			const char c=*bufp++;
			switch(state){
			case 0://method
				if(c==' '){
					state=1;
					pth=bufp;
					qs=0;
				}
				break;
			case 1://uri
				if(c==' '){
					state=2;
					*(bufp-1)=0;
				}else if(c=='?'){
					state=7;
					qs=bufp;
					*(bufp-1)=0;
				}
				break;
			case 7://querystr
				if(c==' '){
					state=2;
					*(bufp-1)=0;
				}
				break;
			case 2://protocol
				if(c=='\n'){
					hdrs.clear();
					hdrp=bufp;
					state=3;
				}
				break;
			case 3://header key
				if(c=='\n'){
					const char*path=pth+1;
					xwriter x=xwriter(fd);
					if(!*path&&qs){
						stats.widgets++;
						widget*o=widgetget(qs);
						try{
							o->to(x);
						}catch(const char*e){
							delete o;
							throw e;
						}
						delete o;
						state=0;
						break;
					}
					if(!*path){
						stats.cache++;
						homepage->to(x);
						state=0;
						break;
					}
					stats.files++;
					struct stat fdstat;
					if(stat(path,&fdstat))
						{x.reply_http(404,"not found");return state=0;}
					const struct tm*tm=localtime(&fdstat.st_mtime);
					char lastmod[64];
					//"Fri, 31 Dec 1999 23:59:59 GMT"
					strftime(lastmod,size_t(64),"%a, %d %b %y %H:%M:%S %Z",tm);
					const char*lastmodstr=hdrs["if-modified-since"];
					if(lastmodstr&&!strcmp(lastmodstr,lastmod)){
						const char*hdr="HTTP/1.1 304\r\nConnection: Keep-Alive\r\n\r\n";
						const ssize_t hdrnn=strlen(hdr);
						const ssize_t hdrsn=send(fd,hdr,hdrnn,0);
						if(hdrsn!=hdrnn)
							{stats.errors++;printf("\n\n%s  %d\n\n",__FILE__,__LINE__);return 0;}
						stats.output+=hdrsn;
						state=0;
						break;
					}
					fdfile=open(path,O_RDONLY);
					if(fdfile==-1)
						{x.reply_http(404,"cannot open");return 0;}
					fdfileoffset=0;
					fdfilecount=fdstat.st_size;
					const char*range=hdrs["range"];
					char bb[K];
					if(range&&*range){
						unsigned long long rs=0;
						if(EOF==sscanf(range,"bytes=%llu",&rs)){
							stats.errors++;
							printf("\n\n%s  %d\n\n",__FILE__,__LINE__);
							return 0;
						}
						fdfileoffset=rs;
						const unsigned long long int s=rs;
						const unsigned long long int e=fdfilecount;
						fdfilecount-=rs;
						sprintf(bb,"HTTP/1.1 206\r\nConnection: Keep-Alive\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %lld\r\nContent-Range: %lld-%lld/%lld\r\n\r\n",lastmod,fdfilecount,s,e,e);
					}else{
						sprintf(bb,"HTTP/1.1 200\r\nConnection: Keep-Alive\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %lld\r\n\r\n",lastmod,fdfilecount);
					}
					const ssize_t bbnn=strlen(bb);
					const ssize_t bbsn=send(fd,bb,bbnn,0);
					if(bbsn!=bbnn)
						{stats.errors++;printf("\n%s:%d sending header\n",__FILE__,__LINE__);return 0;}
					stats.output+=bbsn;//? -1
					const ssize_t nn=sendfile(fd,fdfile,&fdfileoffset,fdfilecount);
					if(nn<0){
						if(errno==32){//broken pipe
							stats.brkp++;
							return 0;
						}
						stats.errors++;
						perror("sending file");
						printf("\n%s:%d errno=%d\n",__FILE__,__LINE__,errno);
						return 0;
					}
					stats.output+=nn;
					fdfilecount-=nn;
					if(fdfilecount!=0){
						state=6;
						return 2;
					}
					close(fdfile);
					state=0;
					break;
				}else if(c==':'){
					*(bufp-1)=0;
					hdrvp=bufp;
					state=4;
				}
				break;
			case 4://header value
				if(c=='\n'){
					*(bufp-1)=0;
					hdrp=strtrm(hdrp,hdrvp-2);
					strlwr(hdrp);
					hdrvp=strtrm(hdrvp,bufp-2);
					hdrs.put(hdrp,hdrvp);
					hdrp=bufp;
					state=3;
				}
				break;
			}
		}
		if(state==0){
			const char*str=hdrs["connection"];
			if(str&&!strcmp("Keep-Alive",str)){
				bufi=bufnn=0;
				bufp=buf;
				return 1;
			}else
				return 0;
		}else
			return 1;
	}
	int read(){
		const ssize_t nn=recv(fd,buf+bufi,conbufnn-bufi,0);
		if(nn==0){//closed
//			printf("\n%s:%d closed by client\n\n",__FILE__,__LINE__);
			return 1;
		}
		if(nn<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK){//error
			if(errno==104){// connection reset by peer
				return 1;
			}
			perror("recv");
			printf("\n%s:%d errno=%d client error\n\n",__FILE__,__LINE__,errno);
			stats.errors++;
			return 1;
		}
		bufnn+=nn;
		stats.input+=nn;
		return 0;
	}
};
static sock server;
//void thdwatchrun(void*arg){
//	if(arg)
//		puts((const char*)arg);
//	stats.printhdr(stdout);
//	while(1){
//		int n=10;
//		while(n--){
//			const int sleep=100000;
//			usleep(sleep);
//			stats.ms+=sleep/1000;
//			stats.print(stdout);
//		}
//		fprintf(stdout,"\n");
//	}
//}
static void sigexit(int i){
	puts("exiting");
	delete homepage;
	if(shutdown(server.fd,SHUT_RDWR))perror("shutdown");
	close(server.fd);
	signal(SIGINT,SIG_DFL);
	kill(getpid(),SIGINT);
	exit(i);
}
//#include<iostream>
//using namespace std;
//static void thdwatchrun(string arg){
//	cout<<arg<<endl;
//	stats.printhdr(stdout);
//	while(1){
//		int n=10;
//		while(n--){
//			const int sleep=100000;
//			usleep(sleep);
//			stats.ms+=sleep/1000;
//			stats.print(stdout);
//		}
//		fprintf(stdout,"\n");
//	}
//}
static void*thdwatchrun(void*arg){
	if(arg)
		puts((const char*)arg);
	stats.printhdr(stdout);
	while(1){
		int n=10;
		while(n--){
			const int sleep=100000;
			usleep(sleep);
			stats.ms+=sleep/1000;
			stats.print(stdout);
		}
		fprintf(stdout,"\n");
	}
	return nullptr;
}
int main(){
	signal(SIGINT,sigexit);
	puts(app);
	printf("  port %d\n",port);

	char buf[4*K];
	snprintf(buf,sizeof buf,"HTTP/1.1 200\r\nConnection: Keep-Alive\r\nContent-Length: %lu\r\n\r\n%s",strlen(app),app);
	homepage=new doc(buf);

	struct sockaddr_in srv;
	const ssize_t srvsz=sizeof(srv);
	bzero(&srv,srvsz);
	srv.sin_family=AF_INET;
	srv.sin_addr.s_addr=INADDR_ANY;
	srv.sin_port=htons(port);
	if((server.fd=socket(AF_INET,SOCK_STREAM,0))==-1)
		{perror("socket");exit(1);}
	if(bind(server.fd,(struct sockaddr*)&srv,srvsz))
		{perror("bind");exit(2);}
	if(listen(server.fd,nclients)==-1)
		{perror("listen");exit(3);}
	const int epfd=epoll_create(nclients);
	if(!epfd)
		{perror("epollcreate");exit(4);}
	struct epoll_event ev;
	ev.events=EPOLLIN;
	ev.data.ptr=&server;
	if(epoll_ctl(epfd,EPOLL_CTL_ADD,server.fd,&ev)<0)
		{perror("epolladd");exit(5);}
	struct epoll_event events[nclients];
	const bool watch_thread=true;
//	thread t1(thd_watch,"hello");
	pthread_t thdwatch;
	if(watch_thread){
		if(pthread_create(&thdwatch,nullptr,&thdwatchrun,nullptr))
			{perror("threadcreate");exit(6);}
	}
	while(true){
		const int nn=epoll_wait(epfd,events,nclients,-1);
		if(nn==-1)
			{perror("epollwait");exit(7);}
		for(int i=0;i<nn;i++){
			sock&c=*(sock*)events[i].data.ptr;
			if(c.fd==server.fd){
				stats.accepts++;
				const int fda=accept(server.fd,0,0);
				if(fda==-1)
					{perror("accept");exit(8);}
				int opts=fcntl(fda,F_GETFL);
				if(opts<0)
					{perror("getopts");exit(9);}
				opts|=O_NONBLOCK;
				if(fcntl(fda,F_SETFL,opts))
					{perror("setopts");exit(10);}
				ev.data.ptr=new sock(fda);
				ev.events=EPOLLIN|EPOLLRDHUP|EPOLLET;
				if(epoll_ctl(epfd,EPOLL_CTL_ADD,fda,&ev))
					{perror("epolladd");exit(11);}
				int flag=1;
				if(setsockopt(fda,IPPROTO_TCP,TCP_NODELAY,(void*)&flag,sizeof(int))<0){
					perror("setsockopt TCP_NODELAY");
					exit(12);
				}
				continue;
			}
			if(events[i].events&EPOLLIN){
				stats.reads++;
				if(c.read()){
					delete&c;
					continue;
				}
			}else
				stats.writes++;
			try{
				switch(c.run()){
				case 0:delete&c;break;
				case 1://read
					events[i].events=EPOLLIN|EPOLLRDHUP|EPOLLET;
					if(epoll_ctl(epfd,EPOLL_CTL_MOD,c.fd,&events[i]))
						{perror("epollmodread");delete&c;}
					break;
				case 2://write
					events[i].events=EPOLLOUT|EPOLLRDHUP|EPOLLET;
					if(epoll_ctl(epfd,EPOLL_CTL_MOD,c.fd,&events[i]))
						{perror("epollmodwrite");delete&c;}
					break;
				}
			}catch(const char*e){
				delete&c;
			}
		}
	}
}
//-- application
namespace web{
class hello:public widget{
	virtual void to(xwriter&x){
		x.reply_http(200,"hello world");
	}
};
class bye:public widget{
	virtual void to(xwriter&x){
		x.reply_http(200,"b y e");
	}
};
class notfound:public widget{
	virtual void to(xwriter&x){
		x.reply_http(200,"path not found");
	}
};}
//-- generated
static widget*widgetget(const char*qs){
	if(!strcmp("hello",qs))
		return new web::hello();
	if(!strcmp("bye",qs))
		return new web::bye();
	return new web::notfound();
}
