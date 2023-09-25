#pragma once
#include"defines.hpp"
#include<errno.h>
#include"conf.hpp"
#include<unistd.h>
namespace xiinux{class chunky final:public xprinter{
	size_t len_{0};
	char buf_[chunky_buf_size_in_bytes];
	int sockfd_;
	inline size_t io_send(const void*ptr,size_t len,bool throw_if_send_not_complete=false){
		sts.writes++;
		const ssize_t n{send(sockfd_,ptr,len,MSG_NOSIGNAL)};
		if(n<0){
			if(errno==EPIPE or errno==ECONNRESET)throw signal_connection_reset_by_peer;
			sts.errors++;
			throw"iosend";
		}
		sts.output+=size_t(n);
		if(conf::print_traffic)write(conf::print_traffic_fd,buf_,n);
		if(throw_if_send_not_complete and size_t(n)!=len){
			sts.errors++;
			throw"sendnotcomplete";
		}
		return size_t(n);
	}
public:
	inline chunky(int sockfd):sockfd_{sockfd}{}
	inline chunky&flush()override{
		if(len_==0)return*this;
		strb b;
		b.p_hex(len_).p(2,"\r\n");
		io_send(b.getbuf(),b.getsize(),true);
		size_t sent_total=0;
		while(true){
			while(true){
				const size_t nsend=len_-sent_total;
				const size_t n=io_send(buf_+sent_total,nsend,false);
				sent_total+=n;
				if(n==nsend)break;
				//blocking
				perr("would block, try again");
			}
			if(sent_total==len_)break;
		}
		io_send("\r\n",sizeof("\r\n")-1,true);
		len_=0;
		return*this;
	}
	inline chunky&finish(){
		flush();
		io_send("0\r\n\r\n",sizeof("0\r\n\r\n")-1,true);
		return*this;
	}
	inline chunky&send_response_header(){
		io_send(buf_,len_,true);
		len_=0;
		return*this;
	}
	inline chunky&p(/*copies*/const char*str)override{
		const size_t n=strnlen(str,sizeof(buf_)+1);//? togetbufferoverrun
		return p(n,str);
	}
	inline chunky&p(const size_t len,/*copies*/const char*str)override{
		const ssize_t sizeofbuf=sizeof(buf_);
		const ssize_t bufrem=sizeofbuf-len_;
		ssize_t rem=bufrem-len;
		if(rem>=0){
			strncpy(buf_+len_,str,len);
			len_+=len;
			return*this;
		}
		strncpy(buf_+len_,str,bufrem);
		len_+=bufrem;
		flush();
		const char*s=str+bufrem;
		rem=-rem;
		while(true){
			const ssize_t n=sizeofbuf-len_;
			const size_t nn=rem<=n?rem:n;
			strncpy(buf_+len_,s,nn);
			len_+=nn;
			flush();
			rem-=nn;
			if(rem==0)
				return*this;
			s+=nn;
		}
	}
	inline chunky&p(const int i)override{
		char str[32];
		const size_t n=snprintf(str,sizeof(str),"%d",i);
		if(n>sizeof(str))throw"snprintf";
		return p(n,str);
	}
	inline chunky&p(const size_t i)override{
		char str[32];
		const size_t n=snprintf(str,sizeof(str),"%zd",i);
		if(n>sizeof(str))throw"snprintf";
		return p(n,str);
	}
	inline chunky&p_ptr(const void*ptr)override{
		char str[32];
		const int n=snprintf(str,sizeof(str),"%p",ptr);
		if(unsigned(n)>sizeof(str))throw"p_ptr:1";
		return p(n,str);
	}
	inline chunky&p_hex(const unsigned long i)override{
		char str[32];
		const int len=snprintf(str,sizeof(str),"%lx",i);
		if(len<0)throw"snprintf";
		return p(len,str);
	}
	inline chunky&p(char ch)override{
		if(sizeof(buf_)-len_==0)flush();
		*(buf_+len_++)=ch;
		return*this;
	}
	inline chunky&nl()override{return p('\n');}
	inline chunky&p(const strb&sb){return p(sb.getsize(),sb.getbuf());}

	// html5
	inline chunky&html5(const char*title="")override{
		const char s[]="<!doctype html><script src=/x.js></script><link rel=stylesheet href=/x.css>";
		return p(sizeof(s),s)
				.p(sizeof("<title>"),"<title>").p(title).p(sizeof("</title>"),"</title>");
	}
	inline chunky&to(FILE*f){
		char fmt[32];
		if(snprintf(fmt,sizeof(fmt),"%%%zus",len_)<1)throw"chunky:err1";
		fprintf(f,fmt,buf_);
		return*this;
	}
};}
