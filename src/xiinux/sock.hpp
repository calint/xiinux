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
namespace xiinux{class sock{
	static sessions sess;
	enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,read_content,upload,next_request};
	parser_state state{next_request};
	struct{
		int fd{0};
		off_t pos{0};
		size_t len{0};
		inline void close(){if(::close(fd)<0)perror("closefile");}
		inline ssize_t resume_send_to(int tofd){
			const ssize_t n=sendfile(tofd,fd,&pos,len);
			if(n<0)return n;
			xiinux::sts.output+=n;
			return n;
//			if(conf::print_trafic)write(conf::print_trafic_fd,p,n);
		}
	}file;
	int upload_fd{0};
	struct{
		char*pth{nullptr};
		char*qs{nullptr};
	}rline;
	lut<const char*>hdrs;
	char*hdrk{nullptr};
	char*hdrv{nullptr};
	class{
	public:
		char*d{nullptr};
		size_t len{0};
		size_t pos{0};
		inline ssize_t receive_from(int fd){return recv(fd,d+pos,len-pos,0);}
		inline size_t rem()const{return len-pos;}
		inline bool is_done()const{return pos==len;}
		inline void rst(){if(d){delete[]d;d=nullptr;}len=pos=0;}
		inline void unsafe_inc_pos(const size_t n){pos+=n;}
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
		inline size_t free_in_buf()const{return sockbuf_size_in_bytes-i;}
		inline size_t rem_to_parse()const{return nn-i;}
		inline void unsafe_inc_len(const size_t n){nn+=n;}
		inline void unsafe_inc_pi(const size_t n){p+=n;i+=n;}
		inline ssize_t receive_from(int fd){
			ssize_t n=recv(fd,p,free_in_buf(),0);
			if(n<0)return n;
			nn=n;
			if(conf::print_trafic)write(conf::print_trafic_fd,p,n);
			return n;
		}
	}buf;
	widget*wdgt{nullptr};
	session*ses{nullptr};
	bool send_session_id_at_next_opportunity{false};
	size_t meter_requests{0};
	//-----  - - - --- --- --- - - -- - -- - -- - - ----- - -- -- - -- - - -
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
		sts.output+=(size_t)nn;
		if(conf::print_trafic)write(conf::print_trafic_fd,buf,nn);
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
	inline void close(){if(::close(fd)<0)perror("sockclosefd");}
	inline void run(){while(true){
		//printf(" state %d\n",state);
		if(state==read_content){
			sts.reads++;
			buf.rst();
			const ssize_t nn=buf.receive_from(fd);
			if(!nn)throw"brk";
			if(nn<0){
				if(errno==EAGAIN||errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw"brk";
				sts.errors++;
				throw"readingcontent";
			}
			sts.input+=(unsigned)nn;
			const size_t crem=content.rem();
			reply x{fd};
			if(crem>(size_t)nn){
				wdgt->on_content(x,buf.ptr(),nn,content.len);
				content.unsafe_inc_pos(nn);
				buf.unsafe_inc_pi(nn);
				if(!content.is_done())
					continue;
				state=next_request;
			}else{
				wdgt->on_content(x,buf.ptr(),crem,content.len);
				content.unsafe_inc_pos(crem);
				buf.unsafe_inc_pi(crem);
				state=next_request;
			}
		}else if(state==upload){
			sts.reads++;
			const ssize_t nn=buf.receive_from(fd);
			if(!nn)throw"brk";
			if(nn<0){
				if(errno==EAGAIN||errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw"brk";
				sts.errors++;
				throw"upload";
			}
			sts.input+=(size_t)nn;
			const size_t crem=content.rem();
			const ssize_t nw=write(upload_fd,buf.ptr(),crem>(size_t)nn?(size_t)nn:crem);
			if(nw<0){sts.errors++;throw"writing upload to file";}
			if(nw!=nn)throw"writing upload to file 2";
			content.unsafe_inc_pos(nw);
			if(!content.is_done())continue;
			if(::close(upload_fd)<0)perr("while closing upload file 2");
			io_send("HTTP/1.1 204\r\n\r\n",16,true);
			state=next_request;
		}else if(state==resume_send_file){
			sts.writes++;
			const ssize_t sf=file.resume_send_to(fd);
			if(sf<0){//error
				if(errno==EAGAIN){io_request_write();return;}
				sts.errors++;
				throw"err";
			}
			file.len-=size_t(sf);
			sts.output+=size_t(sf);
			if(file.len)
				continue;
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
			const ssize_t nn=buf.receive_from(fd);
			if(nn==0){//closed by client
				delete this;
				return;
			}
			if(nn<0){//error
				if(errno==EAGAIN||errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw"brk";
				sts.errors++;
				throw"err";
			}
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
					const char*path=*rline.pth=='/'?rline.pth+1:rline.pth;
					const char*content_length_str=hdrs["content-length"];
					content.rst();
					if(content_length_str)content.len=(size_t)atoll(content_length_str);
					if(!*path and rline.qs){
						sts.widgets++;
						const char*cookie=hdrs["cookie"];
						const char*session_id;
						if(cookie&&strstr(cookie,"i=")){
							//? parse cookie
							session_id=cookie+sizeof "i="-1;
						}else{
							session_id=nullptr;
						}
						if(!session_id){
							// create session
							//"Fri, 31 Dec 1999 23:59:59 GMT"
							time_t timer=time(NULL);
							struct tm* tm_info=gmtime(&timer);
							char* sid=(char*)(malloc(24));
							//						 20150411--225519-ieu44d
							strftime(sid,size_t(24),"%Y%m%d-%H%M%S-",tm_info);
							char* sid_ptr=sid+16;
							for(int i=0;i<7;i++){
								*sid_ptr++='a'+(unsigned char)(random())%26;
							}
							*sid_ptr=0;
							ses=new session(sid);
							sock::sess.put(ses,false);
							send_session_id_at_next_opportunity=true;
						}else{
							ses=sock::sess.get(session_id);
							if(!ses){
								// session not found, reload
								char* sid=(char*)(malloc(64));
								strncpy(sid,session_id,64);
								ses=new session(sid);
								sess.put(ses,false);
							}
						}
						wdgt=ses->get_widget(rline.qs);
						if(!wdgt){
							wdgt=widgetget(rline.qs);
							const size_t key_len=strlen(rline.qs);
							char* key=(char*)(malloc(key_len+1));
							memcpy(key,rline.qs,key_len+1);
							ses->put_widget(key,wdgt);
						}
						reply x=reply(fd);
						if(send_session_id_at_next_opportunity){
							x.send_session_id_at_next_opportunity(ses->id());
							send_session_id_at_next_opportunity=false;
						}
						if(content_length_str){// posting content to widget
							const char*s=hdrs["expect"];
							if(s&&!strcmp(s,"100-continue")){
	//								dbg("client expects 100 continue before sending post");
								io_send("HTTP/1.1 100\r\n\r\n",16,true);
								wdgt->on_content(x,nullptr,0,content.len);//? begin content scan
								state=read_content;
								break;
							}
							const size_t rem=buf.rem_to_parse();
							wdgt->on_content(x,nullptr,0,content.len);//? begin content scan
							if(rem>=content.len){// full content is read in buffer
								wdgt->on_content(x,buf.ptr(),content.len,content.len);
								buf.unsafe_inc_pi(content.len);
								state=next_request;
								break;
							}else{
								wdgt->on_content(x,buf.ptr(),rem,content.len);
								content.unsafe_inc_pos(rem);
								buf.unsafe_inc_pi(rem);
								state=read_content;
								break;
							}
						}else{// requesting widget
							wdgt->to(x);
							state=next_request;
							return;
						}
					}
					const char*content_type=hdrs["content-type"];
					if(content_type&&strstr(content_type,"file")){// file upload
						const mode_t mod{0664};
						char bf[255];
						snprintf(bf,sizeof bf,"upload/%s",rline.pth+1);
						upload_fd=open(bf,O_CREAT|O_WRONLY|O_TRUNC,mod);
						if(upload_fd<0){perror("while creating file for upload");throw"err";}
						const char*s=hdrs["expect"];
						if(s&&!strcmp(s,"100-continue")){
							io_send("HTTP/1.1 100\r\n\r\n",16,true);
							state=upload;
							break;
						}
						const size_t rem=buf.rem_to_parse();
						if(rem==0){
							state=upload;
							break;
						}
						if(rem>=content.len){
							const ssize_t nn=write(upload_fd,buf.ptr(),(size_t)content.len);
							if(nn<0){perr("while writing upload to file");throw"err";}
							sts.input+=(size_t)nn;
							if((size_t)nn!=content.len){throw"incomplete upload";}
							if(::close(upload_fd)<0){perr("while closing upload file");}
							const char resp[]="HTTP/1.1 204\r\n\r\n";
							io_send(resp,sizeof resp-1,true);//. -1 to exclude '\0'
							buf.unsafe_inc_pi(content.len);
							state=next_request;
							break;
						}
						const ssize_t nn=write(upload_fd,buf.ptr(),rem);
						if(nn<0){perror("while writing upload to file2");throw"err";}
						sts.input+=(size_t)nn;
						if((size_t)nn!=rem){throw"upload2";}
						content.unsafe_inc_pos(nn);
						state=upload;
						break;
					}
					reply x{fd};
					if(!*path){
						sts.cache++;
						homepage->to(x);
						state=next_request;
						break;
					}
					if(strstr(path,"..")){
						x.http(403,"path contains ..\n",sizeof "path contains ..\n");
						state=next_request;
						return;
					}
					struct stat fdstat;
					if(stat(path,&fdstat)){
						x.http2(404,"not found\n");
						state=next_request;
						return;
					}
					if(S_ISDIR(fdstat.st_mode)){
						x.http2(403,"path is directory\n");
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
						break;
					}
					file.fd=open(path,O_RDONLY);
					if(file.fd==-1){
						x.http2(404,"cannot open");
						state=next_request;
						break;
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
					const ssize_t nn=file.resume_send_to(fd);
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
						break;
					}
					file.close();
					state=next_request;
					break;
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
					goto read_header_key;
				}
			}
		}
	}}
private:
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
