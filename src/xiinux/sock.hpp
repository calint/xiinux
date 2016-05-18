#pragma once
#include"args.hpp"
#include"widget.hpp"
#include"xiinux.hpp"
#include"../web/web.hpp"
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/sendfile.h>
#include<sys/stat.h>
namespace xiinux{
	class sock{
		static sessions sess;
		enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,read_content,upload,next_request};
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
				perr("ioreqread");
				throw"epollmodread";
			}
		}
		inline void io_request_write(){
			struct epoll_event ev;
			ev.data.ptr=this;
			ev.events=EPOLLOUT;
			if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)){
				perr("ioreqwrite");
				throw"epollmodwrite";
			}
		}
	public:
		int fd{0};
		inline sock(const int f):fd{f}{sts.socks++;}
		inline~sock(){
			sts.socks--;
//			dbg("sock deleted");
			delete[]content;
			if(!::close(fd)){
				return;
			}
			sts.errors++;
			perr("sockdel");
		}
		inline void close(){::close(fd);}
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
//						dbg("eagain || wouldblock");
						io_request_read();
						return;
					}else if(errno==ECONNRESET){
						throw"brk";
					}
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
					perr("while closing file");
				}
				reply::io_send(fd,"HTTP/1.1 204\r\n\r\n",16,true);
	//			printf("     upload done: %s\n",pth+1);
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
//						dbg("eagain || wouldblock");
						io_request_read();
						return;
					}else if(errno==ECONNRESET){
						throw"brk";
					}
					perror("readcontent");
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
//									dbg("client expects 100 continue before sending post");
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
//								dbg("upload fits in buffer");
								const ssize_t nn=write(upload_fd,bufp,(size_t)content_len);
								if(nn<0){
									perr("while writing upload to file");
									throw"err";
								}
								sts.input+=(size_t)nn;
								if((size_t)nn!=content_len){
									throw"incomplete upload";
								}
								if(::close(upload_fd)<0){
									perr("while closing file");
								}
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
						*sid_ptr++='a'+(char)random()%26;
					}
					*sid_ptr=0;
					ses=new session(sid);
//					sess.put(sid,ses,false);
					sess.put(ses,false);
					x.send_session_id_at_next_opportunity(sid);
				}else{
					ses=sess.get(session_id);
					if(!ses){// session not found, reload
						char*sid=(char*)malloc(64);
						strncpy(sid,session_id,64);
						ses=new session(/*gives*/sid);
//						sess.put(sid,ses,false);
						sess.put(ses,false);
					}
				}
				widget*o=ses->get_widget(qs);
				if(!o){
					o=widgetget(qs);
					const size_t key_len=strlen(qs);
					char*key=(char*)malloc(key_len+1);
					memcpy(key,qs,key_len+1);
					ses->put_widget(key,o);
				}
				if(content){
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
					perr("range");
					throw"errrorscanning";
				}
				file_pos=rs;
				const size_t e=file_len;
				file_len-=(size_t)rs;
				bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 206\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",lastmod,file_len,rs,e,e);
			}else{
				// Connection: Keep-Alive\r\n for apache-bench
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
				perr("sendingfile");
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
	};
	sessions sock::sess;
}
