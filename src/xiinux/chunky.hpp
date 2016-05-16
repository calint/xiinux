#ifndef chunky_hpp
#define chunky_hpp
#include"xprinter.hpp"
#include<string.h>
#include<utility>
#include<errno.h>
#include"reply.hpp"
namespace xiinux{
	class chunky:public xprinter{
		size_t length{0};
		char buf[4096];
		int sockfd;
		static inline size_t io_send(int fd,const void*buf,size_t len,bool throw_if_send_not_complete=false){
			sts.writes++;
			const ssize_t n=send(fd,buf,len,MSG_NOSIGNAL);
			if(n<0){
				if(errno==EPIPE||errno==ECONNRESET){
					sts.brkp++;
					throw"brk";
//					throw exception_connection_reset_by_client;
				}
				sts.errors++;
				throw"iosend";
			}
			sts.output+=(size_t)n;
			if(throw_if_send_not_complete&&(size_t)n!=len){
				sts.errors++;
				throw"sendnotcomplete";
			}
			return(size_t)n;
		}
	public:
		inline chunky(int sockfd):sockfd(sockfd){}
//		inline chunky(){}
//		inline chunky(const char*str){p(str);}
//		inline const char*getbuf()const{return buf;}
//		inline size_t getsize()const{return size;}
		inline chunky&flush(){
			char fmt[32];
			const int nn=snprintf(fmt,sizeof fmt,"%zu",length);
			if((unsigned)nn>sizeof fmt+2)
				throw"snprintf";
			fmt[nn]='\r';
			fmt[nn+1]='\n';
			fmt[nn+2]='\0';
			const size_t sent=io_send(sockfd,buf,length,false);
			while(sent!=length){
				// sock.state=waiting_for_write
				// sock::io_request_write()
				// block
				// send rest

			}
			length=0;
//			*buf=0;
			return*this;
		}
		inline chunky&p(/*copies*/const char*str){
			const size_t n=strnlen(str,sizeof buf+1);//. togetbufferoverrun
			return p(n,str);
		}
		inline chunky&p(const size_t len,/*copies*/const char*str){
			const ssize_t rem=sizeof buf-length-len;
			if(rem<0){
				// copy to buf
				strncpy(buf+length,str,len+rem);
				flush();
				strncpy(buf,str+len,-rem);
				return*this;
			}
			strncpy(buf+length,str,len);
			length+=len;
			return*this;
		}
//		#define sp(fmt,value){
//			char str[32];
//			const int n=snprintf(str,sizeof str,fmt,value);
//			if(n>sizeof str)throw"snprintf";
//			return str;
//		}
		inline chunky&p(const int i){
			char str[32];
			const int n=snprintf(str,sizeof str,"%d",i);
			if(n>sizeof str)throw"snprintf";
//			char*str=sp("%d",i);
			return p(n,str);

	//		const ssize_t rem=sizeof buf-size-len;
	//		if(rem<0)throw"bufferoverrun";
	//		strncpy(buf+size,str,len);
	//		size+=len;
	//		return*this;
		}
		inline chunky&p_ptr(const void*ptr){
			char str[32];
			const int n=snprintf(str,sizeof str,"%p",ptr);
			if((unsigned)n>sizeof str)throw"p_ptr:1";
			return p(n,str);
		}
		inline chunky&nl(){
			if(sizeof buf-length<1){
				throw"bufferoverrun2";
				// send buf
				// reset size
			}
			*(buf+length++)='\n';
			return*this;
		}
		inline chunky&p(const strb&sb){
			return p(sb.getsize(),sb.getbuf());
//			const ssize_t rem=sizeof buf-length-sb.size;
//			if(rem<0)throw"bufferoverrun";
//			strncpy(buf+length,sb.buf,sb.size);
//			length+=sb.size;
//			return*this;
		}

		// html5
		inline chunky&html5(const char*title=""){
			const char s[]="<!doctype html><script src=/x.js></script><link rel=stylesheet href=/x.css>";
			return p(sizeof s,s)
					.p(sizeof "<title>","<title>").p(title).p(sizeof "</title>","</title>");
		}
	//	inline strb&title(const char*str){return p(sizeof "<title>","<title>").p(sizeof str,str).p(sizeof "</title>","</title>");}
	//	inline strb&textarea(){
	//		return p("<textarea id=_txt style='width:40em height:10em'>").p(s.getsize(),s.getbuf()).p("</textarea>");
	//
	//	}
		inline chunky&to(FILE*f){
			char fmt[32];
			if(snprintf(fmt,sizeof fmt,"%%%zus",length)<1)throw"err";
			fprintf(f,fmt,buf);
		}
	};
}
#endif
