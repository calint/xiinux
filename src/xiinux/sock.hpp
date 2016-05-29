#pragma once
#include"args.hpp"
#include"widget.hpp"
#include"xiinux.hpp"
#include"conf.hpp"
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
		struct{
			int fd{0};
			off_t pos{0};
			size_t len{0};
			inline void close(){::close(fd);}
			inline ssize_t send_to(int tofd){return sendfile(tofd,fd,&pos,len);}
		}file;
		int upload_fd{0};
		struct{
			char*pth{nullptr};
			char*qs{nullptr};
		}rline;
		lut<const char*>hdrs;
		char*hdrk{nullptr};
		char*hdrv{nullptr};
		struct{
			char*d{nullptr};
			size_t len{0};
			size_t pos{0};
		}content;
		class{
			char d[sockbuf_size_in_bytes];
			char*p{d};
			size_t i{0};
			size_t nn{0};
		public:
			inline bool needs_read()const{return i==nn;}
			inline bool more()const{return i<nn;}
			inline void inci(){i++;}
			inline char unsafe_next_char(){return *p++;}
			inline void eos(){*(p-1)=0;}
			inline void rst(){i=nn=0;p=d;}
			inline char*ptr()const{return p;}
			inline size_t rem_total()const{return sockbuf_size_in_bytes-i;}
			inline size_t rem()const{return nn-i;}
			inline void unsafe_inc_len(const size_t n){nn+=n;}
			inline void unsafe_inc_pi(const size_t n){p+=n;i+=n;}
		}buf;
		size_t meter_requests{0};
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
		inline size_t io_send(const void*buf,size_t len,bool throw_if_send_not_complete=false){
			sts.writes++;
			const ssize_t nn=send(fd,buf,len,MSG_NOSIGNAL);
			if(nn<0){
				if(errno==EPIPE||errno==ECONNRESET){
					sts.brkp++;
					throw"brk";
//					throw exception_connection_reset_by_client;
				}
				sts.errors++;
				throw"iosend";
			}
			if(conf::print_trafic){
				write(conf::print_trafic_fd,buf,len);
			}
			sts.output+=(size_t)nn;
			if(throw_if_send_not_complete&&(size_t)nn!=len){
				sts.errors++;
				throw"sendnotcomplete";
			}
			return(size_t)nn;
		}
	public:
		int fd{0};
		inline sock(const int f):fd{f}{sts.socks++;}
		inline~sock(){
			sts.socks--;
//			dbg("sock deleted");
			delete[]content.d;
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
				char upload_buffer[upload_stack_buf_size_in_bytes];
				const size_t upload_remaining=content.len-content.pos;
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
				if(conf::print_trafic){
					write(conf::print_trafic_fd,upload_buffer,nn);
				}
				sts.input+=(size_t)nn;
				content.pos+=(size_t)nn;
	//			printf(" uploading %s   %zu of %zu\n",pth+1,content_pos,content_len);
				if(content.pos<content.len){
					continue;
				}
				if(::close(upload_fd)<0){
					perr("while closing file");
				}
				io_send("HTTP/1.1 204\r\n\r\n",16,true);
	//			printf("     upload done: %s\n",pth+1);
				state=next_request;
			}else if(state==read_content){
				sts.reads++;
				const ssize_t nn=recv(fd,content.d+content.pos,content.len-content.pos,0);
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
				if(conf::print_trafic){
					write(conf::print_trafic_fd,content.d+content.pos,nn);
				}
				content.pos+=(unsigned)nn;
				sts.input+=(unsigned)nn;
				if(content.pos==content.len){
					*(content.d+content.len)=0;
					process();
					break;
				}
				return;
			}else if(state==resume_send_file){
				sts.writes++;
				const ssize_t sf=file.send_to(fd);
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
				file.len-=size_t(sf);
				sts.output+=size_t(sf);
				if(file.len!=0){
					io_request_write();
					return;
				}
				file.close();
				state=next_request;
			}
			if(meter_requests){
				auto connection=hdrs["connection"];
				if(connection and !strcmp("close",connection)){
					delete this;
					return;
				}
			}
			if(buf.needs_read()){//? assumes request and headers fit in conbufnn and done in one read
//				if(buf.i>=sockbuf_size_in_bytes)
//					throw"reqbufoverrun";//? chained requests buf pointers
				if(state==next_request){//? next_request
					buf.rst();
				}
				sts.reads++;
				const ssize_t nn=recv(fd,buf.ptr(),buf.rem_total(),0);
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
				if(conf::print_trafic){
					write(conf::print_trafic_fd,buf.ptr(),nn);
				}
				buf.unsafe_inc_len((size_t)nn);
//				printf("%s : %s",__PRETTY_FUNCTION__,buf);fflush(stdout);
				sts.input+=(unsigned)nn;
			}
			if(state==next_request){
				sts.requests++;
				meter_requests++;
				state=method;
			}
			if(state==method){
				while(buf.more()){
					buf.inci();
					const char c=buf.unsafe_next_char();
					if(c==' '){
						state=uri;
						rline.pth=buf.ptr();
						rline.qs=nullptr;
						break;
					}
				}
			}
			if(state==uri){
				while(buf.more()){
					buf.inci();
					const char c=buf.unsafe_next_char();
					if(c==' '){
						state=protocol;
						buf.eos();
						urldecode(rline.pth);
						break;
					}else if(c=='?'){
						state=query;
						rline.qs=buf.ptr();
						buf.eos();
						break;
					}
				}
			}
			if(state==query){
				while(buf.more()){
					buf.inci();
					const char c=buf.unsafe_next_char();
					if(c==' '){
						state=protocol;
						buf.eos();
						urldecode(rline.qs);
						break;
					}
				}
			}
			if(state==protocol){
				while(buf.more()){
					buf.inci();
					const char c=buf.unsafe_next_char();
					if(c=='\n'){
						hdrs.clear();
						hdrk=buf.ptr();
						state=header_key;
						break;
					}
				}
			}
			if(state==header_key){
	read_header_key:
				while(buf.more()){
					buf.inci();
					const char c=buf.unsafe_next_char();
					if(c=='\n'){// content or done parsing
						const char*content_length_str=hdrs["content-length"];
						if(!content_length_str){
							content.d=nullptr;
							content.len=0;//? already
							process();
							break;
						}
						content.len=(size_t)atoll(content_length_str);
						const char*content_type=hdrs["content-type"];
						if(content_type&&strstr(content_type,"file")){// file upload
	//							printf("uploading file: %s   size: %s\n",pth+1,content_length_str);
							const mode_t mod{0664};
							char bf[255];
							snprintf(bf,sizeof bf,"upload/%s",rline.pth+1);
							upload_fd=open(bf,O_CREAT|O_WRONLY|O_TRUNC,mod);
							if(upload_fd<0){
								perror("while creating file for upload");
								throw"err";
							}
							const char*s=hdrs["expect"];
							if(s&&!strcmp(s,"100-continue")){
//									dbg("client expects 100 continue before sending post");
								io_send("HTTP/1.1 100\r\n\r\n",16,true);
								state=upload;
								break;
							}
							const size_t chars_left_in_buffer=buf.rem();
							if(chars_left_in_buffer==0){
								state=upload;
								break;
							}
							if(chars_left_in_buffer>=content.len){
//								dbg("upload fits in buffer");
								const ssize_t nn=write(upload_fd,buf.ptr(),(size_t)content.len);
								if(nn<0){
									perr("while writing upload to file");
									throw"err";
								}
								sts.input+=(size_t)nn;
								if((size_t)nn!=content.len){
									throw"incomplete upload";
								}
								if(::close(upload_fd)<0){
									perr("while closing file");
								}
								const char resp[]="HTTP/1.1 204\r\n\r\n";
								io_send(resp,sizeof resp-1,true);//. -1 to remove eos
								buf.unsafe_inc_pi(content.len);
								state=next_request;
								break;
							}
							const ssize_t nn=write(fd,buf.ptr(),chars_left_in_buffer);
							if(nn<0){
								perror("while writing upload to file2");
								throw"err";
							}
							content.pos=(size_t)nn;
							state=upload;
							break;
						}
						// posted content
						delete[]content.d;
						content.d=new char[content.len+1];// extra char for end-of-string
						const size_t chars_left_in_buffer=buf.rem();
						if(chars_left_in_buffer>=content.len){
							memcpy(content.d,buf.ptr(),(size_t)content.len);
							*(content.d+content.len)=0;
							buf.unsafe_inc_pi(content.len);
							process();
							break;
						}else{
							memcpy(content.d,buf.ptr(),chars_left_in_buffer);
							content.pos=chars_left_in_buffer;
							const char*s=hdrs["expect"];
							if(s&&!strcmp(s,"100-continue")){
								io_send("HTTP/1.1 100\r\n\r\n",16,true);
							}
							state=read_content;
							break;
						}
					}else if(c==':'){
						buf.eos();
						hdrv=buf.ptr();
						state=header_value;
						break;
					}
				}
			}
			if(state==header_value){
				while(buf.more()){
					buf.inci();
					const char c=buf.unsafe_next_char();
					if(c=='\n'){
						buf.eos();
						hdrk=strtrm(hdrk,hdrv-2);
						strlwr(hdrk);
						hdrv=strtrm(hdrv,buf.ptr()-2);
						hdrs.put(hdrk,hdrv);
						hdrk=buf.ptr();
						state=header_key;
	//					break;
						goto read_header_key;
					}
				}
			}
		}}
	private:
		void process(){
			const char*path=*rline.pth=='/'?rline.pth+1:rline.pth;
			reply x=reply(fd);
			if(!*path and rline.qs){
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
						*sid_ptr++='a'+(unsigned char)random()%26;
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
				widget*o=ses->get_widget(rline.qs);
				if(!o){
					o=widgetget(rline.qs);
					const size_t key_len=strlen(rline.qs);
					char*key=(char*)malloc(key_len+1);
					memcpy(key,rline.qs,key_len+1);
					ses->put_widget(key,o);
				}
				if(content.d){
					o->on_content(x,content.d,content.len);
					delete[]content.d;
					content.d=nullptr;
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
				io_send(hdr,hdrnn,true);
				state=next_request;
				return;
			}
			file.fd=open(path,O_RDONLY);
			if(file.fd==-1){
				x.http2(404,"cannot open");
				state=next_request;
				return;
			}
			sts.files++;
			file.pos=0;
			file.len=size_t(fdstat.st_size);
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
				file.pos=rs;
				const size_t e=file.len;
				file.len-=(size_t)rs;
				bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 206\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",lastmod,file.len,rs,e,e);
			}else{
				// Connection: Keep-Alive\r\n for apache-bench
				bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 200\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\n\r\n",lastmod,file.len);
			}
			if(bb_len<0)throw"err";
			io_send(bb,(size_t)bb_len,true);
			const ssize_t nn=file.send_to(fd);
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
			file.len-=size_t(nn);
			if(file.len!=0){
				state=resume_send_file;
				io_request_write();
				return;
			}
			file.close();
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
