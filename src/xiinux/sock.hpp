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
	enum parser_state{method,uri,query,protocol,header_key,header_value,resume_send_file,receiving_content,receiving_upload,next_request};
	parser_state state{next_request};

	class{
		off_t pos{0};
		size_t len{0};
		int fd{0};
	public:
		inline void close(){if(::close(fd)<0)perror("closefile");}
		inline ssize_t resume_send_to(int tofd){
			const ssize_t n=sendfile(tofd,fd,&pos,len-pos);
			if(n<0)return n;
			xiinux::sts.output+=n;
			return n;
//			if(conf::print_trafic)write(conf::print_trafic_fd,p,n);
		}
		inline void init_for_send(const size_t size_in_bytes,const off_t seek_pos=0){
			pos=seek_pos;
			len=size_in_bytes;
		}
		inline bool done()const{return (size_t)pos==len;}
		inline size_t length()const{return len;}
		inline int open(const char*path){fd=::open(path,O_RDONLY);return fd;}
		inline void rst(){fd=pos=len=0;}
	}file;

	struct{
		char*pth{nullptr};
		char*qs{nullptr};
		inline void rst(){pth=qs=nullptr;}
	}rline;

	lut<const char*>hdrs;

	struct{
		char*hdrk{nullptr};
		char*hdrv{nullptr};
		inline void rst(){hdrk=hdrv=nullptr;}
	}parserstate;

	class{
		size_t pos{0};
		size_t len{0};
	public:
		inline size_t rem()const{return len-pos;}
		inline bool is_done()const{return pos==len;}
		inline void rst(){len=pos=0;}
		inline void unsafe_skip(const size_t n){pos+=n;}
		inline size_t total_length()const{return len;}
		inline void init_for_receive(const char*content_length_str){len=(size_t)atoll(content_length_str);pos=0;}
	}content;

	int upload_fd{0};

	class{
		char d[sockbuf_size_in_bytes];
		char*p{d};
		char*e{d};
	public:
		inline bool needs_read()const{return p==e;}
		inline bool more()const{return p!=e;}
		inline char unsafe_next_char(){return *p++;}
		inline void eos(){*(p-1)=0;}
		inline void rst(){p=e=d;}
		inline char*ptr()const{return p;}
//		inline size_t avail_buf()const{return sockbuf_size_in_bytes-(p-d);}
		inline size_t rem()const{return e-p;}
		inline void unsafe_skip(const size_t n){p+=n;}
		inline ssize_t receive_from(int fd){
			ssize_t n=recv(fd,p,sockbuf_size_in_bytes-(p-d),0);
			if(n<0)return n;
			e=d+n;
			sts.input+=(unsigned)n;
			if(conf::print_trafic)write(conf::print_trafic_fd,p,n);
			return n;
		}
	}buf;

	widget*wdgt{nullptr};
	session*ses{nullptr};
	bool send_session_id_in_reply{false};

	size_t meter_requests{0};
	//-----  - - - --- --- --- - - -- - -- - -- - - ----- - -- -- - -- - - -
//	inline void init_for_new_request(){
//		file.rst();
//		rline.rst();
//		hdrs.clear();
//		parserstate.rst();
//		content.rst();
//		upload_fd=0;
////		buf.rst();
//		wdgt=nullptr;
//		ses=nullptr;
//		send_session_id_in_reply=false;
//	}
	inline void io_request_read(){
		struct epoll_event ev;
		ev.data.ptr=this;
		ev.events=EPOLLIN;
		if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev))throw"epollmodread";
	}
	inline void io_request_write(){
		struct epoll_event ev;
		ev.data.ptr=this;
		ev.events=EPOLLOUT;
		if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev))throw"epollmodwrite";
	}
	inline size_t io_send(const void*buf,size_t len,bool throw_if_send_not_complete=false){
		sts.writes++;
		const ssize_t nn=send(fd,buf,len,MSG_NOSIGNAL);
		if(nn<0){
			if(errno==EPIPE or errno==ECONNRESET){
				sts.brkp++;
				throw"brk";
			}
			sts.errors++;
			throw"iosend";
		}
		sts.output+=(size_t)nn;
		if(conf::print_trafic)write(conf::print_trafic_fd,buf,nn);
		if(throw_if_send_not_complete and size_t(nn)!=len){
			sts.errors++;
			throw"sendnotcomplete";
		}
		return size_t(nn);
	}
public:
	int fd{0};
	inline sock(const int f):fd{f}{sts.socks++;}
	inline~sock(){
		sts.socks--;
//			dbg("sock deleted");
//		delete[]content.d;
		if(!::close(fd)){
			return;
		}
		sts.errors++;
		perr("sockdel");
	}
	inline void close(){if(::close(fd)<0)perror("sockclosefd");}
	inline void run(){while(true){
		//printf(" state %d\n",state);
		if(state==receiving_content){
			sts.reads++;
			buf.rst();
			const ssize_t nn=buf.receive_from(fd);
			if(!nn)throw"brk";
			if(nn<0){
				if(errno==EAGAIN or errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw"brk";
				sts.errors++;
				throw"readingcontent";
			}
			const size_t crem=content.rem();
			const size_t total=content.total_length();
			reply x{fd};
			if(crem>(size_t)nn){
				wdgt->on_content(x,buf.ptr(),nn,total);
				content.unsafe_skip(nn);
				buf.unsafe_skip(nn);
				if(!content.is_done())
					continue;
				state=next_request;
			}else{
				wdgt->on_content(x,buf.ptr(),crem,total);
				content.unsafe_skip(crem);
				buf.unsafe_skip(crem);
				state=next_request;
			}
		}else if(state==receiving_upload){
			sts.reads++;
			const ssize_t nn=buf.receive_from(fd);
			if(!nn)throw"brk";
			if(nn<0){
				if(errno==EAGAIN or errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw"brk";
				sts.errors++;
				throw"upload";
			}
			const size_t crem=content.rem();
			const ssize_t nw=write(upload_fd,buf.ptr(),crem>(size_t)nn?(size_t)nn:crem);
			if(nw<0){sts.errors++;throw"writing upload to file";}
			if(nw!=nn)throw"writing upload to file 2";
			content.unsafe_skip(nw);
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
			if(file.done())
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
		if(state==next_request){
			sts.requests++;
			meter_requests++;
			file.rst();
			rline.rst();
			hdrs.clear();
			parserstate.rst();
			content.rst();
			upload_fd=0;
	//		buf.rst();
			wdgt=nullptr;
			ses=nullptr;
//			send_session_id_in_reply=false;
			state=method;
		}
		if(buf.needs_read()){//? assumes request and headers fit in conbufnn and done in one read
//			if(buf.i>=sockbuf_size_in_bytes)throw"reqbufoverrun";//? chained requests buf pointers
			buf.rst();
			sts.reads++;
			const ssize_t nn=buf.receive_from(fd);
			if(nn==0){//closed by client
				delete this;
				return;
			}
			if(nn<0){//error
				if(errno==EAGAIN or errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw"brk";
				sts.errors++;
				throw"err";
			}
		}
		if(state==method){
			while(buf.more()){
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
				const char c=buf.unsafe_next_char();
				if(c=='\n'){
					hdrs.clear();
					parserstate.hdrk=buf.ptr();
					state=header_key;
					break;
				}
			}
		}
		if(state==header_key){
read_header_key:
			while(buf.more()){
				const char c=buf.unsafe_next_char();
				if(c=='\n'){// content or done parsing
					const char*path=*rline.pth=='/'?rline.pth+1:rline.pth;
					const char*content_length_str=hdrs["content-length"];
					content.rst();
					if(content_length_str)content.init_for_receive(content_length_str);
					if(!*path and rline.qs){
						sts.widgets++;
						const char*cookie=hdrs["cookie"];
						const char*session_id;
						if(cookie and strstr(cookie,"i=")){
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
							send_session_id_in_reply=true;
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
						if(send_session_id_in_reply){
							x.send_session_id_at_next_opportunity(ses->id());
							send_session_id_in_reply=false;
						}
						if(content_length_str){// posting content to widget
							const size_t total=content.total_length();
							const char*s=hdrs["expect"];
							if(s and !strcmp(s,"100-continue")){
	//								dbg("client expects 100 continue before sending post");
								io_send("HTTP/1.1 100\r\n\r\n",16,true);
								wdgt->on_content(x,nullptr,0,total);//? begin content scan
								state=receiving_content;
								break;
							}
							wdgt->on_content(x,nullptr,0,total);//? begin content scan
							const size_t rem=buf.rem();
							if(rem>=total){// full content is read in buffer
								wdgt->on_content(x,buf.ptr(),total,total);
								buf.unsafe_skip(total);
								state=next_request;
								break;
							}else{
								wdgt->on_content(x,buf.ptr(),rem,total);
								content.unsafe_skip(rem);
								buf.unsafe_skip(rem);
								state=receiving_content;
								break;
							}
						}else{// requesting widget
							wdgt->to(x);
							state=next_request;
							return;
						}
					}
					const char*content_type=hdrs["content-type"];
					if(content_type and strstr(content_type,"file")){// file upload
						const mode_t mod{0664};
						char bf[255];
						if(snprintf(bf,sizeof bf,"upload/%s",rline.pth+1)==sizeof bf)throw"pathtrunc";
						if((upload_fd=open(bf,O_CREAT|O_WRONLY|O_TRUNC,mod))<0){perror("while creating file for upload");throw"err";}
						const char*s=hdrs["expect"];
						if(s and !strcmp(s,"100-continue")){
							io_send("HTTP/1.1 100\r\n\r\n",16,true);
							state=receiving_upload;
							break;
						}
						const size_t rem=buf.rem();
						if(rem==0){
							state=receiving_upload;
							break;
						}
						const size_t total=content.total_length();
						if(rem>=total){
							const ssize_t nn=write(upload_fd,buf.ptr(),(size_t)total);
							if(nn<0){perr("while writing upload to file");throw"err";}
							if((size_t)nn!=total){throw"incomplete upload";}
							if(::close(upload_fd)<0){perr("while closing upload file");}
							const char resp[]="HTTP/1.1 204\r\n\r\n";
							io_send(resp,sizeof resp-1,true);//. -1 to exclude '\0'
							buf.unsafe_skip(total);
							state=next_request;
							break;
						}
						const ssize_t nn=write(upload_fd,buf.ptr(),rem);
						if(nn<0){perror("while writing upload to file2");throw"err";}
						if((size_t)nn!=rem){throw"upload2";}
						content.unsafe_skip(nn);
						state=receiving_upload;
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
						x.http(403,"path contains ..\n",sizeof "path contains ..\n"-1);
						state=next_request;
						return;
					}
					struct stat fdstat;
					if(stat(path,&fdstat)){
						x.http(404,"not found\n",sizeof "not found\n"-1);
						state=next_request;
						return;
					}
					if(S_ISDIR(fdstat.st_mode)){
						x.http(403,"path is directory\n",sizeof "path is directory\n"-1);
						state=next_request;
						return;
					}
					const struct tm*tm=gmtime(&fdstat.st_mtime);
					char lastmod[64];
					//"Fri, 31 Dec 1999 23:59:59 GMT"
					strftime(lastmod,sizeof lastmod,"%a, %d %b %y %H:%M:%S %Z",tm);
					const char*lastmodstr=hdrs["if-modified-since"];
					if(lastmodstr and !strcmp(lastmodstr,lastmod)){
						const char hdr[]="HTTP/1.1 304\r\n\r\n";
						const size_t hdrnn=sizeof hdr;
						io_send(hdr,hdrnn,true);
						state=next_request;
						break;
					}
					if(file.open(path)<0){
						x.http(404,"cannot open\n",sizeof "cannot open\n"-1);
						state=next_request;
						break;
					}
					sts.files++;
					const char*range=hdrs["range"];
					char bb[K];
					int bb_len;
					if(range and *range){
						off_t rs{0};
						if(EOF==sscanf(range,"bytes=%jd",&rs)){
							sts.errors++;
							perr("range");
							throw"errrorscanning";
						}
						file.init_for_send(size_t(fdstat.st_size),rs);
						const size_t e=file.length();
						bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 206\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",lastmod,e-rs,rs,e,e);
					}else{
						// Connection: Keep-Alive\r\n for apache-bench
						file.init_for_send(size_t(fdstat.st_size));
						bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 200\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\n\r\n",lastmod,file.length());
					}
					if(bb_len==sizeof bb)throw"err";
					io_send(bb,(size_t)bb_len,true);
					const ssize_t nn=file.resume_send_to(fd);
					if(nn<0){
						if(errno==EPIPE or errno==ECONNRESET){
							sts.brkp++;
							throw"brk";
						}
						sts.errors++;
						perr("sendingfile");
						throw"err";
					}
					if(!file.done()){
						state=resume_send_file;
						break;
					}
					file.close();
					state=next_request;
					break;
				}else if(c==':'){
					buf.eos();
					parserstate.hdrv=buf.ptr();
					state=header_value;
					break;
				}
			}
		}
		if(state==header_value){
			while(buf.more()){
				const char c=buf.unsafe_next_char();
				if(c=='\n'){
					buf.eos();
					parserstate.hdrk=strtrm(parserstate.hdrk,parserstate.hdrv-2);
					strlwr(parserstate.hdrk);
					parserstate.hdrv=strtrm(parserstate.hdrv,buf.ptr()-2);
					hdrs.put(parserstate.hdrk,parserstate.hdrv);
					parserstate.hdrk=buf.ptr();
					state=header_key;
					goto read_header_key;
				}
			}
		}
	}}
private:
	static inline char*strtrm(char*p,char*e){
		while(p!=e and isspace(*p))
			p++;
		while(p!=e and isspace(*e))
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
