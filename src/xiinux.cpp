#define APP "xiinux web server"
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
#include<typeinfo>
#include<functional>
//class widget;
//static widget*widgetget(const char*qs);
namespace xiinux{
	static int const K=1024;
	static size_t const conbufnn=K;
	static int const nclients=K;
	static int const port=8088;
	class stats{
	public:
		size_t ms{0};
		size_t socks{0};
		size_t sessions{0};
		size_t requests{0};
		size_t input{0};
		size_t output{0};
		size_t accepts{0};
		size_t reads{0};
		size_t writes{0};
		size_t files{0};
		size_t widgets{0};
		size_t cache{0};
		size_t errors{0};
		size_t brkp{0};
		void printhdr(FILE*f){
			fprintf(f,"%12s%12s%12s%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s%8s\n","ms","input","output","socks","reqs","sess","accepts","reads","writes","files","widgets","cache","errors","brkp");
			fflush(f);
		}
		void print(FILE*f){
			fprintf(f,"\r%12zu%12zu%12zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu%8zu",ms,input,output,socks,requests,sessions,accepts,reads,writes,files,widgets,cache,errors,brkp);
			fflush(f);
		}
	};
	static stats stats;
	static const char*exception_connection_reset_by_client="brk";
	static size_t io_send(int fd,const void*buf,size_t len,bool throw_if_send_not_complete=false){
		stats.writes++;
		const ssize_t n=send(fd,buf,len,MSG_NOSIGNAL);
		if(n<0){
			if(errno==EPIPE||errno==ECONNRESET){
				stats.brkp++;
	//			throw"brk";
				throw exception_connection_reset_by_client;
			}
			stats.errors++;
			throw"iosend";
		}
		stats.output+=(size_t)n;
		if(throw_if_send_not_complete&&(size_t)n!=len){
			stats.errors++;
			throw"sendnotcomplete";
		}
		return(size_t)n;
	}
	class xprinter{
	public:
		virtual~xprinter(){};
		virtual xprinter&p(/*scan*/const char*str)=0;
		virtual xprinter&p(const size_t len,/*scan*/const char*str)=0;
		virtual xprinter&p(const int i)=0;
		virtual xprinter&p_ptr(const void*ptr)=0;
		virtual xprinter&nl()=0;
	//	virtual xprinter&p(const strb&sb)=0;
		virtual xprinter&html5(const char*title="")=0;
	};
	class strb:public xprinter{
		size_t size{0};
		char buf[4096];
	//	strb*nxt{nullptr};
	public:
		inline strb(){}
		inline strb(const char*str){p(str);}
		inline const char*getbuf()const{return buf;}
		inline size_t getsize()const{return size;}
		inline strb&rst(){size=0;return*this;}
		inline strb&p(/*copies*/const char*str){
			const size_t len=strnlen(str,sizeof buf+1);//. togetbufferoverrun
			const ssize_t rem=sizeof buf-size-len;
			if(rem<0)throw"bufferoverrun";
			strncpy(buf+size,str,len);
			size+=len;
			return*this;
		}
		inline strb&p(const size_t len,/*copies*/const char*str){
			const ssize_t rem=sizeof buf-size-len;
			if(rem<0)throw"bufferoverrun";
			strncpy(buf+size,str,len);
			size+=len;
			return*this;
		}
		inline strb&p(const int i){
			char str[32];
			const int len=snprintf(str,sizeof str,"%d",i);
			if(len<0)throw"snprintf";
			return p(len,str);
	//		const ssize_t rem=sizeof buf-size-len;
	//		if(rem<0)throw"bufferoverrun";
	//		strncpy(buf+size,str,len);
	//		size+=len;
	//		return*this;
		}
		inline strb&p_ptr(const void*ptr){
			char str[32];
			const int len=snprintf(str,sizeof str,"%p",ptr);
			if(len<0)throw"p_ptr:1";
			return p(len,str);
		}
		inline strb&nl(){
			if(sizeof buf-size<1)throw"bufferoverrun2";
			*(buf+size++)='\n';
			return*this;
		}
		inline strb&p(const strb&sb){
			const ssize_t rem=sizeof buf-size-sb.size;
			if(rem<0)throw"bufferoverrun";
			strncpy(buf+size,sb.buf,sb.size);
			size+=sb.size;
			return*this;
		}

		// html5
		inline strb&html5(const char*title=""){
			const char s[]="<!doctype html><script src=/x.js></script><link rel=stylesheet href=/x.css>";
			return p(sizeof s,s)
					.p(sizeof "<title>","<title>").p(title).p(sizeof "</title>","</title>");
		}
	//	inline strb&title(const char*str){return p(sizeof "<title>","<title>").p(sizeof str,str).p(sizeof "</title>","</title>");}
	//	inline strb&textarea(){
	//		return p("<textarea id=_txt style='width:40em height:10em'>").p(s.getsize(),s.getbuf()).p("</textarea>");
	//
	//	}
		inline strb&to(FILE*f){
			char fmt[32];
			if(snprintf(fmt,sizeof fmt,"%%%zus",size)<1)throw"err";
			fprintf(f,fmt,buf);
		}
	};

	class reply{
		int fd;
		const char*set_session_id{nullptr};
	//	inline reply&pk(const char*s){
	//		const size_t snn=strlen(s);
	//		return pk(s,snn);
	//	}
	public:
		reply(const int fd=0):fd(fd){}
		inline reply&pk(const char*s,const size_t nn){
			io_send(fd,s,nn,true);
			return*this;
		}
		inline void send_session_id_at_next_opportunity(const char*id){set_session_id=id;}
		reply&http(const int code,const char*content,const size_t len){
			char bb[K];
			int n;
			if(set_session_id){
				// Connection: Keep-Alive\r\n  for apache bench
	//			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\nSet-Cookie: i=%s;Expires=Wed, 09 Jun 2021 10:18:14 GMT\r\n\r\n",code,len,set_session_id);
				n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nContent-Length: %zu\r\nSet-Cookie: i=%s;Expires=Wed, 09 Jun 2021 10:18:14 GMT\r\n\r\n",code,len,set_session_id);
				set_session_id=nullptr;
			}else{
	//			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\n\r\n",code,len);
				n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nContent-Length: %zu\r\n\r\n",code,len);
			}
			if(n<0)throw"send";
			pk(bb,(size_t)n).pk(content,len);
			return*this;
		}
		reply&http(const int code,const strb&s){
			return http(code,s.getbuf(),s.getsize());
		}
		reply&http2(const int code,const char*content){
			const size_t nn=strlen(content);
			return http(code,content,nn);
		}
	};
	class doc{
		size_t size;
		char*buf;
	//	const char*lastmod;
	public:
		doc(const char*data,const char*lastmod=nullptr){//:lastmod(lastmod){
	//		printf("new doc %p\n",(void*)this);
			size=strlen(data); //? overrun
			buf=(char*)malloc(size);
			memcpy(buf,data,size);
		}
		~doc(){
	//		printf(" * delete doc %p\n",(void*)this);
			free(buf);
	//		delete buf;
		}
		inline const char*getbuf()const{return buf;}
		inline size_t getsize()const{return size;}
		inline void to(reply&x)const{x.pk(buf,size);}
	};
	static doc*homepage;

	class widget{
	public:
		virtual~widget(){
	//		printf(" * delete widget %s  @  %p\n",typeid(*this).name(),(void*)this);
		};
		virtual void to(reply&x)=0;
	//	virtual void ax(reply&x,char*a[]=0){if(a)x.pk(a[0]);}
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len){};
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
		unsigned int size;
		class el{
		public:
			char*key{nullptr};
			T data{nullptr};
			el*nxt{nullptr};
			el(char*key,T data):key(key),data(data){
	//			printf(" * new lut element %s @ %p\n",key,(void*)this);
			}
			~el(){
	//			printf("delete lut element %s  %s @ %p\n",key,typeid(*this).name(),(void*)this);
	//			printf("delete lut element %s  @ %p\n",key,(void*)this);
	//			printf("delete lut element @ %p\n",(void*)this);
				if(nxt)
					delete nxt;
			}
			void delete_content_recurse(bool delete_key){
	//			printf("delete lut data %s @ %p\n",typeid(*data).name(),(void*)this);
				if(data)
					delete data;
				if(delete_key)
					free(key);
				if(!nxt)
					return;
				nxt->delete_content_recurse(delete_key);
			}
		};
		el**array;
	public:
		static unsigned int hash(const char*key,const unsigned int roll){
			unsigned int i=0;
			const char*p=key;
			while(*p)
				i+=(unsigned int)*p++;
			i%=roll;
			return i;
		}
		lut(const unsigned int size=8):size(size){
	//		printf("new lut %p\n",(void*)this);
			array=(el**)calloc(size_t(size),sizeof(el*));
		}
		~lut(){
	//		printf("delete lut %p\n",(void*)this);
			clear();
			free(array);
		}
		T operator[](const char*key){
			const unsigned int h=hash(key,size);
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
		void put(char*key,T data,bool allow_overwrite=true){
			const unsigned int h=hash(key,size);
			el*l=array[h];
			if(!l){
				array[h]=new el(key,data);
				return;
			}
			while(1){
				if(!strcmp(l->key,key)){
					if(!allow_overwrite)
						throw"lutoverwrite";
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
	//		printf("clear lut %p\n",(void*)this);
			for(unsigned int i=0;i<size;i++){
				el*e=array[i];
				if(!e)
					continue;
				delete e;
				array[i]=nullptr;
			}
		}
		void delete_content(const bool delete_keys){
	//		printf("delete lut content %p\n",(void*)this);
			for(unsigned int i=0;i<size;i++){
				el*e=array[i];
				if(!e)
					continue;
				e->delete_content_recurse(delete_keys);
				delete e;
				array[i]=nullptr;
			}
		}
	};
//	#include<functional>
	template<class T>class lst{
	private:
		class el{
		public:
			T ptr{0};
			el*nxt{nullptr};
			inline el(T ptr):ptr(ptr),nxt(nullptr){}
		};
		el*first{nullptr};
		el*last{nullptr};
		size_t size{0};
		inline void clr(){
			el*e=first;
			while(e){
				el*ee=e;
				delete ee;
				e=e->nxt;
			}
		}
	public:
		inline lst(){}
		inline~lst(){clr();}
		inline void clear(){clr();first=last=nullptr;}
		inline void to(FILE*f)const{
			fprintf(f,"[first=%p;last=%p][",first,last);
			el*e=first;
			while(e){
				fprintf(f,"%p",e);
				e=e->nxt;
				if(e)
					fprintf(f," ");
			}
			fprintf(f,"]\n");
		}
		inline void add(T ptr){
			size++;
			el*e=new el(ptr);
			if(!last){
				first=last=e;
				return;
			}
			last=last->nxt=e;
		}
		inline void addfirst(T ptr){
			size++;
			el*e=new el(ptr);
			if(!first){
				first=last=e;
				return;
			}
			e->nxt=first;
			first=e;
		}
		inline T take_first(){
			if(first==nullptr)
				return nullptr;
			size--;
			T ret=first->ptr;
			if(!first->nxt){
				delete first;
				first=last=nullptr;
				return ret;
			}
			el*e=first;
			first=first->nxt;
			delete e;
			return ret;
		}
		inline T find(T key){
			el*e=first;
			while(e){
				if(e->ptr==key)
					return e->ptr;
				e=e->nxt;
			}
			return nullptr;
		}
		inline size_t getsize()const{return size;}
		inline bool isempty()const{return size==0;}
		void foreach(bool f(T)){
			if(!first)
				return;
			el*e=first;
			while(e){
				if(!f(e->ptr))
					break;
				e=e->nxt;
			}
		}
		void foreach2(const std::function<bool (T)>&f){
			if(!first)
				return;
			el*e=first;
			while(e){
				if(!f(e->ptr))
					break;
				e=e->nxt;
			}
		}
	};
	class session{
		char*_id;
		lut<char*>kvp;
		lut<widget*>widgets;
	public:
		session(/*takes*/char*session_id):_id(session_id){
			stats.sessions++;
	//		printf(" * new session %s @ %p\n",session_id,(void*)this);
		}
		~session(){
			stats.sessions--;
	//		printf(" * delete session %s\n",_id);
			free(_id);
			kvp.delete_content(true);
			widgets.delete_content(true);
		}
		inline const char*id()const{return _id;}
		inline void*operator[](const char*key){return kvp[key];}
		inline void put(char*key,/*takes*/char*data){kvp.put(key,data);}
		inline widget*get_widget(const char*key){return widgets[key];}
		inline void put_widget(char*key,/*takes*/widget*o){widgets.put(key,o);}
	};
	class sessions{
	public:
		~sessions(){
	//		printf(" * delete sessions %p\n",(void*)this);
			all.delete_content(false);
		}
		lut<session*>all{K};
	};
	static sessions sessions;
//	static widget*widgetget(const char*qs);
	static int epollfd;
	static void urldecode(char*str){
		const char*p=str;
		while(*p){
			char a,b;
			if(*p=='%'&&(a=p[1])&&(b=p[2])&&isxdigit(a)&&isxdigit(b)){
				if(a>='a')a-='a'-'A';//?
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
	#define dbg(str) printf("%s:%d %s   %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,str);
	class sock{
	//	enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,read_content,upload,next_request};
		enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,read_content,upload};
		parser_state state{method};
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
		void io_request_read(){
			struct epoll_event ev;
			ev.data.ptr=this;
			ev.events=EPOLLIN;
			if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)){
				perror("ioreqread");
				throw"epollmodread";
			}
		}
		void io_request_write(){
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
		sock(const int f):fd(f){
			stats.socks++;
			printf("%s:%d %s : new %d\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,fd);
		}
		~sock(){
			stats.socks--;
			printf("%s:%d %s : delete %d\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,fd);
			//printf(" * delete sock %p\n",(void*)this);
			delete[]content;
			if(!::close(fd)){
				return;
			}
			stats.errors++;
	//		printf("%s:%d ",__FILE__,__LINE__);perror("sockdel");
			printf("%s:%d %s : sockdel\n",__FILE__,__LINE__,__PRETTY_FUNCTION__);
			perror("sockdel");
		}
		void close(){
			::close(fd);
		}
		void run(){while(true){
			//printf(" state %d\n",state);
			if(state==upload){
				char upload_buffer[4*K];
				const size_t upload_remaining=content_len-content_pos;
				stats.reads++;
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
					stats.errors++;
					throw"upload";
				}
				const ssize_t nw=write(upload_fd,upload_buffer,(size_t)nn);
				if(nw<0){
					printf("\n\n%s:%d ",__FILE__,__LINE__);
					perror("writing upload to file");
					stats.errors++;
					throw"writing upload to file";
				}
				if(nw!=nn){
					throw"writing upload to file 2";
				}
				stats.input+=(size_t)nn;
				content_pos+=(size_t)nn;
	//			printf(" uploading %s   %zu of %zu\n",pth+1,content_pos,content_len);
				if(content_pos<content_len){
					continue;
				}
				if(::close(upload_fd)<0){
					perror("while closing file");
				}
	//			printf("      done %s\n",pth+1);
				io_send(fd,"HTTP/1.1 204\r\n\r\n",16,true);
				state=method;
			}else if(state==read_content){
				stats.reads++;
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
					stats.errors++;
					throw"readingcontent";
				}
				content_pos+=(unsigned)nn;
				stats.input+=(unsigned)nn;
				if(content_pos==content_len){
					*(content+content_len)=0;
					process();
					break;
				}
				return;
			}else if(state==resume_send_file){
				stats.writes++;
				const ssize_t sf=sendfile(fd,file_fd,&file_pos,file_len);
				if(sf<0){//error
					if(errno==EAGAIN){
						io_request_write();
						return;
					}
					stats.errors++;
					printf("\n\n%s:%d ",__FILE__,__LINE__);perror("resumesendfile");
					delete this;
					return;
				}
				file_len-=size_t(sf);
				stats.output+=size_t(sf);
				if(file_len!=0){
	//				state=resume_send_file;
					io_request_write();
					return;
				}
				::close(file_fd);
				state=method;
			}
			if(bufi==bufnn){//? assumes request and headers fit in conbufnn and done in one read
				if(bufi>=conbufnn)
					throw"reqbufoverrun";//? chained requests buf pointers
				if(state==method){//? next_request
					bufi=bufnn=0;
					bufp=buf;
				}
				stats.reads++;
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
					printf("\n\n%s:%d ",__FILE__,__LINE__);perror("readbuf");
					stats.errors++;
					delete this;
					return;
				}
				bufnn+=(size_t)nn;
				printf("%s : %s",__PRETTY_FUNCTION__,buf);fflush(stdout);
				stats.input+=(unsigned)nn;
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
								io_send(fd,"HTTP/1.1 100\r\n\r\n",16,true);
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
								stats.input+=(size_t)nn;
								if((size_t)nn!=content_len){
									throw"incomplete upload";
								}
								if(::close(upload_fd)<0){
									perror("while closing file");
								}
	//							const char resp[]="HTTP/1.1 200\r\nContent-Length: 0\r\n\r\n";
								const char resp[]="HTTP/1.1 204\r\n\r\n";
								io_send(fd,resp,sizeof resp-1,true);//. -1 to remove eos
								bufp+=content_len;
								bufi+=content_len;
								state=method;
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
								io_send(fd,"HTTP/1.1 100\r\n\r\n",16,true);
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
				stats.widgets++;
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
					sessions.all.put(sid,ses,false);
					x.send_session_id_at_next_opportunity(sid);
				}else{
					ses=sessions.all[session_id];
					if(!ses){// session not found, reload
	//					printf(" * session not found, recreating: %s\n",session_id);
						char*sid=(char*)malloc(64);
	//					if(strlen(session_id)>23)throw"cookielen";
						strncpy(sid,session_id,64);
		//				printf(" * creating session %s\n",session_id);
						ses=new session(sid);
						sessions.all.put(sid,ses,false);
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
					state=method;
					return;
				}else{
					o->to(x);
					state=method;
					return;
				}
			}
			if(!*path){
				stats.cache++;
				homepage->to(x);
				state=method;
				return;
			}
			if(strstr(path,"..")){
				x.http2(403,"path contains ..");
				state=method;
				return;
			}
			stats.files++;
			struct stat fdstat;
			if(stat(path,&fdstat)){
				x.http2(404,"not found");
				state=method;
				return;
			}
			if(S_ISDIR(fdstat.st_mode)){
				x.http2(403,"path is directory");
				state=method;
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
				io_send(fd,hdr,hdrnn,true);
				state=method;
				return;
			}
			file_fd=open(path,O_RDONLY);
			if(file_fd==-1){
				x.http2(404,"cannot open");
				state=method;
				return;
			}
			file_pos=0;
			file_len=size_t(fdstat.st_size);
			const char*range=hdrs["range"];
			char bb[K];
			int bb_len;
			if(range&&*range){
				off_t rs=0;
				if(EOF==sscanf(range,"bytes=%zu",&rs)){
					stats.errors++;
					printf("\n\n%s:%d ",__FILE__,__LINE__);perror("scanrange");
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
			io_send(fd,bb,(size_t)bb_len,true);
			const ssize_t nn=sendfile(fd,file_fd,&file_pos,file_len);
			if(nn<0){
				if(errno==EPIPE||errno==ECONNRESET){
					stats.brkp++;
					throw"brk";
				}
				stats.errors++;
				printf("\n\n%s:%d ",__FILE__,__LINE__);perror("sendfile");
				throw"err";
			}
			stats.output+=size_t(nn);
			file_len-=size_t(nn);
			if(file_len!=0){
				state=resume_send_file;
				io_request_write();
				return;
			}
			::close(file_fd);
			state=method;
		}
	};
	static sock server_socket(0);
	static void*thdwatchrun(void*arg){
		if(arg)
			puts((const char*)arg);
		stats.printhdr(stdout);
		while(1){
			int n=10;
			while(n--){
				const int sleep=100000;
				usleep(sleep);
				stats.ms+=sleep/1000;//? not really
				stats.print(stdout);
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
					stats.accepts++;
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
					if(msg!=exception_connection_reset_by_client){
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
//-- application
namespace web{
	using namespace xiinux;
	class hello:public widget{
		virtual void to(reply&x)override{
			x.http2(200,"hello world");
		}
	};
	class bye:public widget{
		virtual void to(reply&x)override{
			x.http2(200,"b y e");
		}
	};
	class notfound:public widget{
		virtual void to(reply&x)override{
			x.http2(404,"path not found");
		}
	};
	class typealine:public widget{
		virtual void to(reply&x)override{
			x.http2(200,"typealine");
		}
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
	//		printf(" typealine received content: %s\n",content);
			x.http(200,content,content_len);
		}
	};
	class counter:public widget{
		int c;
		virtual void to(reply&x)override{
			strb sb;sb.p("counter ").p(++c).nl();
	//		strb().p("counter ").p(++c).nl();
			x.http(200,sb.getbuf(),sb.getsize());
		}
	};
	class notes:public widget{
		strb txt;
	public:
		notes(){
			txt.p("notebook\n");
		}
		virtual void to(reply&x)override{
			strb s;
			s.html5("notebook")
					.p("<input id=_btn type=button value=save onclick=\"this.disabled=true;ajax_post('/?notes',$('_txt').value,function(r){console.log(r);$('_btn').disabled=false;eval(r.responseText);})\"><br>\n")
					.p("<textarea id=_txt class=big>")
					.p(txt)
					.p("</textarea>")
					.p("<script>$('_txt').focus()</script>")
					.nl();
			x.http(200,s.getbuf(),s.getsize());
		}
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
			txt.
				rst().
				p(content_len,content);
			x.http2(200,"location.reload();");
		}
	};
	class a:public widget{
		/*ref*/a*pt{nullptr};// parent
		/*own*/const char*nm{nullptr};// name
	public:
		a(/*refs*/a*parent,/*takes*/const char*name):pt(parent),nm(name){
			printf("%s:%d %s  #    new  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
		}
		virtual~a(){
			printf("%s:%d %s  # delete  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
			delete nm;
		};
		inline a*getparent()const{return pt;}
		inline void setparent(a*p){pt=p;}
		inline void setname(/*takes*/const char*name){if(nm)delete(nm);nm=name;}
		inline const char*getname()const{return nm;}
		virtual void to(reply&x){
			strb s;
	//		s.p(__FILE__).p(":").p(__LINE__).p(" ").p(__PRETTY_FUNCTION__).p("  ");
			s.p(typeid(*this).name()).p("@").p_ptr(this);
			x.http(200,s);
		}
	};
	class an:public a{
	public:
		const char*id;
		virtual~an(){
			printf("%s:%d %s  # delete  %s  @  %p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
			delete id;
		};
		virtual void to(reply&x)=0;
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len){};
	};

	class page:public a{
		strb txt;
	//	lst<page*>sub;
	public:
		page(a*parent,/*takes*/const char*name):a(parent,name){
			printf("%s:%d %s  # new page  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
		}
		virtual void to(reply&x)override{
			strb s;
			s.html5("page")
					.p("<input id=_btn type=button value=update onclick=\"this.disabled=true;ajax_post('/?page',$('_txt').value,function(r){console.log(r);$('_btn').disabled=false;eval(r.responseText);})\">")
					.p("\n")
					.p("<textarea id=_txt class=big>")
					.p(txt)
					.p("</textarea>")
					.p("<script>$('_txt').focus()</script>")
					.nl();
			x.http(200,s);
		}
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
			txt.
			rst().
				p(content_len,content);
			x.http2(200,"location.reload();");
		}
	};
}//namespace web

//-- generated
namespace xiinux{
	static widget*widgetget(const char*qs){
		if(!strcmp("hello",qs))return new web::hello();
		if(!strcmp("typealine",qs))return new web::typealine();
		if(!strcmp("bye",qs))return new web::bye();
		if(!strcmp("counter",qs))return new web::counter();
		if(!strcmp("notes",qs))return new web::notes();
		if(!strcmp("page",qs))return new web::page(nullptr,nullptr);
		return new web::notfound();
	}
}
int main(int c,char**a){
	return xiinux::main(c,a);
}

