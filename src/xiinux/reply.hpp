#pragma once
#include"stats.hpp"
#include"strb.hpp"
#include<sys/socket.h>
#include<errno.h>
#include<string.h>
#include"chunky.hpp"
#include"conf.hpp"
#include<unistd.h>
namespace xiinux{class reply{
	int fd;
	const char*set_session_id_cookie{nullptr};
public:
	inline reply(const int fd=0):fd{fd}{}
	inline /*give*/chunky*reply_chunky(){
		chunky*c=new chunky(fd);
		return c;
	}
	inline size_t io_send(const void*buf,size_t len,bool throw_if_send_not_complete=false){
		sts.writes++;
		const ssize_t n{send(fd,buf,len,MSG_NOSIGNAL)};
		if(n<0){
			if(errno==EPIPE or errno==ECONNRESET)throw signal_connection_reset_by_peer;
			sts.errors++;
			throw"iosend";
		}
		sts.output+=size_t(n);
		if(conf::print_trafic)write(conf::print_trafic_fd,buf,n);
		if(throw_if_send_not_complete and size_t(n)!=len){
			sts.errors++;
			throw"sendnotcomplete";
		}
		return(size_t)n;
	}
	inline reply&pk(const char*data,const size_t len){
		io_send(data,len,true);
		return*this;
	}
	inline void send_session_id_at_next_opportunity(const char*id){set_session_id_cookie=id;}
	inline reply&http(const int code,const char*content=nullptr,size_t len=0){
		char bb[1024];
		if(content and !len)len=strnlen(content,K*M);
		int n;
		if(set_session_id_cookie){
			// Connection: Keep-Alive\r\n  for apache bench
//			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\nSet-Cookie: i=%s;path=/;expires=Thu, 31-Dec-2099 00:00:00 GMT;SameSite=Lax\r\n\r\n",code,len,set_session_id);
			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nContent-Length: %zu\r\nSet-Cookie: i=%s;path=/;expires=Thu, 31-Dec-2099 00:00:00 GMT;SameSite=Lax\r\n\r\n",code,len,set_session_id_cookie);
			set_session_id_cookie=nullptr;
		}else{
//			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nConnection: Keep-Alive\r\nContent-Length: %zu\r\n\r\n",code,len);
			n=snprintf(bb,sizeof bb,"HTTP/1.1 %d\r\nContent-Length: %zu\r\n\r\n",code,len);
		}
		if(n<0)throw"send";
		io_send(bb,n,true);
		if(content)io_send(content,len,true);
		return*this;
	}
	inline reply&http(const int code,const strb&s){
		return http(code,s.getbuf(),s.getsize());
	}
	inline reply&http2(const int code,const char*str){
		const size_t nn=strlen(str);
		return http(code,str,nn);
	}
};}
