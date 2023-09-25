#pragma once
#include"args.hpp"
#include"widget.hpp"
#include"xiinux.hpp"
#include"conf.hpp"
#include"sessions.hpp"
#include"../web/web.hpp"
#include<sys/socket.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/sendfile.h>
#include<sys/stat.h>
namespace xiinux{class sock{
	enum{method,uri,query,protocol,header_key,header_value,resume_send_file,receiving_content,receiving_upload,next_request}state{method};

	class{
		off_t pos{0};
		size_t len{0};
		int fd{0};
	public:
		inline void close(){if(::close(fd)<0)perror("closefile");}
		inline ssize_t resume_send_to(int tofd){
			const ssize_t n{sendfile(tofd,fd,&pos,len-pos)};
			if(n<0)return n;
			xiinux::sts.output+=n;
			return n;
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
	}reqline;

	lut<const char*>hdrs;

	struct{
		char*key{nullptr};
		char*value{nullptr};
		inline void rst(){key=value=nullptr;}
	}hdrparser;

	class{
		size_t pos{0};
		size_t len{0};
		char*buf{nullptr};
	public:
		inline void rst(){pos=len=0;}
		inline void cleanup(){
			if(!buf)
				return;
			delete[]buf;
			buf=nullptr;
		}
		inline bool more()const{return pos!=len;}
		inline size_t rem()const{return len-pos;}
		inline void unsafe_skip(const size_t n){pos+=n;}

		inline size_t total_length()const{return len;}
		inline void init_for_receive(const char*content_length_str){
			pos=0;
			len=(size_t)atoll(content_length_str);
			if(!buf){
				buf=new char[sock_content_buf_size_in_bytes];
			}
		}
		inline char*ptr()const{return buf;}
		inline ssize_t receive_from(int fd){
			sts.reads++;
			const ssize_t n{recv(fd,buf,sock_content_buf_size_in_bytes,0)};
			if(n<0)return n;
			sts.input+=(unsigned)n;
			if(conf::print_traffic)write(conf::print_traffic_fd,buf,(unsigned)n);
			return n;
		}
	}content;

	class{
		char d[sock_req_buf_size_in_bytes];
		char*p{d};
		char*e{d};
	public:
		inline void rst(){p=e=d;}
		inline bool more()const{return p!=e;}
		inline size_t rem()const{return e-p;}
		inline void unsafe_skip(const size_t n){p+=n;}
		inline char unsafe_next_char(){return *p++;}
		inline void eos(){*(p-1)=0;}
		inline char*ptr()const{return p;}
		inline ssize_t receive_from(int fd){
			const unsigned nbytes_to_read=sock_req_buf_size_in_bytes-(p-d);
			if(nbytes_to_read==0)
				throw"sock:buf:full";
			sts.reads++;
			const ssize_t n{recv(fd,p,nbytes_to_read,0)};
			if(n<0)return n;
			sts.input+=(unsigned)n;
			e=p+n;
			if(conf::print_traffic)write(conf::print_traffic_fd,p,(unsigned)n);
			return n;
		}
	}buf;

	int upload_fd{0};
	widget*wdgt{nullptr};
	session*ses{nullptr};
	bool send_session_id_in_reply{false};

	//-----  - - - --- --- --- - - -- - -- - -- - - ----- - -- -- - -- - - -
	inline void io_request_read(){
		struct epoll_event ev;
		ev.data.ptr=this;
		ev.events=EPOLLIN;
		if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev))throw"sock:epollmodread";
	}
	inline void io_request_write(){
		struct epoll_event ev;
		ev.data.ptr=this;
		ev.events=EPOLLOUT;
		if(epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev))throw"sock:epollmodwrite";
	}
	inline size_t io_send(const void*buf,size_t len,bool throw_if_send_not_complete=false){
		sts.writes++;
		const ssize_t n{send(fd,buf,len,MSG_NOSIGNAL)};
		if(n<0){
			if(errno==EPIPE or errno==ECONNRESET)throw signal_connection_reset_by_peer;
			sts.errors++;
			throw"sock:iosend";
		}
		const size_t un{size_t(n)};
		sts.output+=un;
		if(conf::print_traffic)write(conf::print_traffic_fd,buf,un);
		if(throw_if_send_not_complete and un!=len){
			sts.errors++;
			throw"sock:sendnotcomplete";
		}
		return un;
	}
public:
	int fd{0};
	inline sock(const int f=0):fd{f}{sts.socks++;}
	inline~sock(){
		content.cleanup();
		sts.socks--;
		if(!::close(fd))return;
		sts.errors++;
		perr("sockdel");
	}
	inline void run(){while(true){
		//printf(" state %d\n",state);
		if(state==receiving_content){
			const ssize_t n{content.receive_from(fd)};//?? thrashes pointers used in request line and headers
			if(n==0)throw signal_connection_reset_by_peer;
			if(n<0){
				if(errno==EAGAIN or errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw signal_connection_reset_by_peer;
				sts.errors++;
				throw"sock:receiving_content";
			}
			const size_t un{size_t(n)};
			const size_t crem{content.rem()};
			const size_t total{content.total_length()};
			reply x{fd};
			if(crem>un){
				wdgt->on_content(x,content.ptr(),un,total);
				content.unsafe_skip(un);
				continue;
			}
			wdgt->on_content(x,content.ptr(),crem,total);
			content.unsafe_skip(crem);
			state=next_request;
		}else if(state==receiving_upload){
			const ssize_t n{buf.receive_from(fd)};//?? thrashes pointers used in request line and headers
			if(!n)throw signal_connection_reset_by_peer;
			if(n<0){
				if(errno==EAGAIN or errno==EWOULDBLOCK){io_request_read();return;}
				else if(errno==ECONNRESET)throw signal_connection_reset_by_peer;
				sts.errors++;
				throw"sock:receiving_upload";
			}
			const size_t un{size_t(n)};
			const size_t crem{content.rem()};
			const ssize_t m{write(upload_fd,buf.ptr(),crem>un?un:crem)};
			if(m<0){sts.errors++;throw"sock:writing upload to file";}
			if(m!=n)throw"sock:writing upload to file 2";
			content.unsafe_skip((unsigned)m);
			if(content.more())continue;
			if(::close(upload_fd)<0)perr("while closing upload file 2");
			io_send("HTTP/1.1 204\r\n\r\n",16,true);// 16 is the length of string
			state=next_request;
		}else if(state==resume_send_file){
			sts.writes++;
			const ssize_t sf{file.resume_send_to(fd)};
			if(sf<0){// error or send buffer full
				if(errno==EAGAIN){
					io_request_write();
					return;
				}
				sts.errors++;
				throw"sock:err2";
			}
			if(!file.done())
				continue;
			file.close();
			state=next_request;
		}
		if(state==next_request){
			// if previous request had header 'Connection: close'
			const char*connection{hdrs["connection"]};
			if(connection and !strcmp("close",connection)){
				delete this;
				return;
			}
			sts.requests++;
			file.rst();
			reqline.rst();
			hdrs.clear();
			hdrparser.rst();
			content.rst();
			upload_fd=0;
			wdgt=nullptr;
			ses=nullptr;
			buf.rst();
			state=method;
		}
		if(!buf.more()){
			//?? assumes request header fits in buf and done in one read
			//   then parsing can be simplified
			const ssize_t nn{buf.receive_from(fd)};
			if(nn==0){// closed by client
				delete this;
				return;
			}
			if(nn<0){// error or would block
				if(errno==EAGAIN or errno==EWOULDBLOCK){
					io_request_read();
					return;
				}else if(errno==ECONNRESET){
					throw signal_connection_reset_by_peer;
				}
				sts.errors++;
				throw"sock:err3";
			}
		}
		if(state==method){
			while(buf.more()){
				const char c{buf.unsafe_next_char()};
				if(c==' '){
					state=uri;
					reqline.pth=buf.ptr();
					break;
				}
			}
		}
		if(state==uri){
			while(buf.more()){
				const char c{buf.unsafe_next_char()};
				if(c==' '){
					buf.eos();
					state=protocol;
					break;
				}else if(c=='?'){
					buf.eos();
					reqline.qs=buf.ptr();
					state=query;
					break;
				}
			}
		}
		if(state==query){
			while(buf.more()){
				const char c{buf.unsafe_next_char()};
				if(c==' '){
					buf.eos();
					state=protocol;
					break;
				}
			}
		}
		if(state==protocol){
			while(buf.more()){
				const char c{buf.unsafe_next_char()};
				if(c=='\n'){
					hdrparser.key=buf.ptr();
					state=header_key;
					break;
				}
			}
		}
		if(state==header_key){
read_header_key:
			while(buf.more()){
				const char c{buf.unsafe_next_char()};
				if(c=='\n'){// content or done parsing
					const char*path{*reqline.pth=='/'?reqline.pth+1:reqline.pth};
					// printf("path: '%s'\nquery: '%s'\n",path,reqline.qs);
					const char*content_length_str{hdrs["content-length"]};
					content.rst();
					if(content_length_str)content.init_for_receive(content_length_str);
					if(!*path and reqline.qs){
						sts.widgets++;
						const char*cookie{hdrs["cookie"]};
						const char*session_id{nullptr};
						if(cookie and strstr(cookie,"i=")){//? parse cookie
							session_id=cookie+sizeof "i="-1;
						}
						if(!session_id){
							// create session
							// "Fri, 31 Dec 1999 23:59:59 GMT"
							time_t timer{time(NULL)};
							struct tm*tm_info{gmtime(&timer)};
							char*sid{new char[24]};
							// 20150411--225519-ieu44d
							strftime(sid,size_t(24),"%Y%m%d-%H%M%S-",tm_info);
							char*sid_ptr{sid+16};
							for(int i=0;i<7;i++){
								*sid_ptr++='a'+(unsigned char)(random())%26;
							}
							*sid_ptr=0;
							ses=new session(/*give*/sid);
							sess.put(ses,false);
							send_session_id_in_reply=true;
						}else{
							ses=sess.get(session_id);
							if(!ses){
								// session not found, reload
								char* sid{new char[64]};
								strncpy(sid,session_id,64);
								ses=new session(/*give*/sid);
								sess.put(/*give*/ses,false);
							}
						}
						wdgt=ses->get_widget(reqline.qs);
						if(!wdgt){
							wdgt=widgetget(reqline.qs);
							const size_t key_len{strlen(reqline.qs)};
							char* key{new char[key_len+1]}; // +1 for the \0 terminator
							memcpy(key,reqline.qs,key_len+1);
							ses->put_widget(/*give*/key,/*give*/wdgt);
						}
						reply x=reply(fd);
						if(send_session_id_in_reply){
							x.send_session_id_at_next_opportunity(ses->id());
							send_session_id_in_reply=false;
						}
						if(content_length_str){// posting content to widget
							const size_t total{content.total_length()};
							wdgt->on_content(x,nullptr,0,total);
							const char*s{hdrs["expect"]};
							if(s and !strcmp(s,"100-continue")){
								// dbg("client expects 100 continue before sending post");
								io_send("HTTP/1.1 100\r\n\r\n",16,true);
								state=receiving_content;
								break;
							}
							const size_t rem{buf.rem()};
							if(rem>=total){// full content is in 'buf'
								wdgt->on_content(x,buf.ptr(),total,total);
								state=next_request;
								break;
							}else{
								// part of the content is in 'buf'
								wdgt->on_content(x,buf.ptr(),rem,total);
								content.unsafe_skip(rem);
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
						char bf[256];
						if(snprintf(bf,sizeof bf,"upload/%s",reqline.pth+1)==sizeof bf)throw"sock:pathtrunc";// +1 to skip the leading '/'
						if((upload_fd=open(bf,O_CREAT|O_WRONLY|O_TRUNC,mod))<0){perror("while creating file for upload");throw"sock:err7";}
						const char*s{hdrs["expect"]};
						if(s and !strcmp(s,"100-continue")){
							io_send("HTTP/1.1 100\r\n\r\n",16,true);// 16 is string length
							state=receiving_upload;
							break;
						}
						const size_t rem{buf.rem()};
						if(rem==0){
							state=receiving_upload;
							break;
						}
						const size_t total{content.total_length()};
						if(rem>=total){
							const ssize_t n{write(upload_fd,buf.ptr(),(size_t)total)};
							if(n<0){perr("while writing upload to file");throw"sock:err4";}
							if((size_t)n!=total){throw"sock:incomplete upload";}
							if(::close(upload_fd)<0){perr("while closing upload file");}
							const char resp[]{"HTTP/1.1 204\r\n\r\n"};
							io_send(resp,sizeof resp-1,true);// -1 to exclude '\0'
							buf.unsafe_skip(total);
							state=next_request;
							break;
						}
						const ssize_t n{write(upload_fd,buf.ptr(),rem)};
						if(n<0){perror("while writing upload to file2");throw"sock:err6";}
						if((size_t)n!=rem){throw"upload2";}
						content.unsafe_skip(n);
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
					const struct tm*tm{gmtime(&fdstat.st_mtime)};
					{	char lastmod[64];
						// "Fri, 31 Dec 1999 23:59:59 GMT"
						strftime(lastmod,sizeof lastmod,"%a, %d %b %y %H:%M:%S %Z",tm);
						const char*lastmodstr{hdrs["if-modified-since"]};
						if(lastmodstr and !strcmp(lastmodstr,lastmod)){
							const char hdr[]{"HTTP/1.1 304\r\n\r\n"};
							const size_t hdrnn{sizeof hdr};
							io_send(hdr,hdrnn,true);
							state=next_request;
							break;
						}
						if(file.open(path)<0){
							x.http(404,"cannot open\n",sizeof "cannot open\n"-1); // -1 ignore the '\0'
							state=next_request;
							break;
						}
						sts.files++;
						const char*range{hdrs["range"]};
						char bb[K];
						int bb_len;
						if(range and *range){
							off_t rs{0};
							if(EOF==sscanf(range,"bytes=%jd",&rs)){//? is sscanf safe
								sts.errors++;
								perr("range");
								throw"sock:errrorscanning";
							}
							file.init_for_send(size_t(fdstat.st_size),rs);
							const size_t e{file.length()};
							bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 206\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\nContent-Range: %zu-%zu/%zu\r\n\r\n",lastmod,e-rs,rs,e,e);
							// puts(bb);
						}else{
							file.init_for_send(size_t(fdstat.st_size));
							// "Connection: Keep-Alive\r\n" for apache-bench
							bb_len=snprintf(bb,sizeof bb,"HTTP/1.1 200\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Length: %zu\r\n\r\n",lastmod,file.length());
							// puts(bb);
						}
						if(bb_len==sizeof bb)throw"sock:err1";
						io_send(bb,size_t(bb_len),true);
					}
					const ssize_t nn{file.resume_send_to(fd)};
					if(nn<0){
						if(errno==EPIPE or errno==ECONNRESET)throw signal_connection_reset_by_peer;
						sts.errors++;
						perr("sendingfile");
						throw"sock:err5";
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
					hdrparser.value=buf.ptr();
					state=header_value;
					break;
				}
			}
		}
		if(state==header_value){
			while(buf.more()){
				const char c{buf.unsafe_next_char()};
				if(c=='\n'){
					buf.eos();
					hdrparser.key=strtrm(hdrparser.key,hdrparser.value-2);// -2 to skip '\0' and place pointer on last character in the key
					strlwr(hdrparser.key);// RFC 2616: header field names are case-insensitive
					hdrparser.value=strtrm(hdrparser.value,buf.ptr()-2);//? -2 to skip '\0' and place pointer on last character in the value
					// printf("%s: %s\n",hdrparser.key,hdrparser.value);
					hdrs.put(hdrparser.key,hdrparser.value);
					hdrparser.key=buf.ptr();
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
}srv;}
