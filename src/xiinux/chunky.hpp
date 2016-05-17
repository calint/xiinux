#ifndef chunky_hpp
#define chunky_hpp
#include"xprinter.hpp"
#include<string.h>
#include<utility>
#include<errno.h>
#include"reply.hpp"
namespace xiinux{
	class chunky:public xprinter{
		#define loop()while(true)
		size_t length{0};
		char buf[256];
		int sockfd;
		static inline size_t io_send(int fd,const void*buf,size_t len,bool throw_if_send_not_complete=false){
			sts.writes++;
			const ssize_t n=::send(fd,buf,len,MSG_NOSIGNAL);
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
//		inline~chunky(){
//			puts("delete chunky");
//		}
//		inline chunky(){}
//		inline chunky(const char*str){p(str);}
//		inline const char*getbuf()const{return buf;}
//		inline size_t getsize()const{return size;}
		inline chunky&flush(){
			if(length==0)return*this;
			strb b;
			b.p_hex(length).p(2,"\r\n");
//			const auto bf=b.getbuf();
//			const auto ln=b.getsize();
			io_send(sockfd,b.getbuf(),b.getsize(),true);
			size_t sent_total=0;
			loop(){
				loop(){
					const size_t nsend=length-sent_total;
					const size_t n=io_send(sockfd,buf+sent_total,nsend,false);
					sent_total+=n;
					if(n==nsend)break;
					//blocking
					perror("would block");
//					perror("sock.state=waiting_for_write");
//					perror("sock::io_request_write()");
//					perror("wait");//? racing
//					perror("sent+=io_send(...)");//? racing
				}
				if(sent_total==length)break;
			}
			io_send(sockfd,"\r\n",sizeof "\r\n"-1,true);
			length=0;
			return*this;
		}
		inline chunky&finish(){
			flush();
			io_send(sockfd,"0\r\n\r\n",sizeof "0\r\n\r\n"-1,true);
			return*this;
		}
		inline chunky&send_response_header(){
			io_send(sockfd,buf,length,true);
			length=0;
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
				const size_t fit=len+rem;
				//? if fit<0
				strncpy(buf+length,str,fit);
				length+=fit;
				flush();
				//? while rem>sizeof buf   send chunks
				strncpy(buf,str+fit,-rem);
				length+=-rem;
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
			const size_t n=snprintf(str,sizeof str,"%d",i);
			if(n>sizeof str)throw"snprintf";
//			char*str=sp("%d",i);
			return p(n,str);

	//		const ssize_t rem=sizeof buf-size-len;
	//		if(rem<0)throw"bufferoverrun";
	//		strncpy(buf+size,str,len);
	//		size+=len;
	//		return*this;
		}
		inline chunky&p(const size_t i){
			char str[32];
			const size_t n=snprintf(str,sizeof str,"%zd",i);
			if(n>sizeof str)throw"snprintf";
			return p(n,str);
		}
		inline chunky&p_ptr(const void*ptr){
			char str[32];
			const int n=snprintf(str,sizeof str,"%p",ptr);
			if((unsigned)n>sizeof str)throw"p_ptr:1";
			return p(n,str);
		}
		inline chunky&p_hex(const long long i){
			char str[32];
			const int len=snprintf(str,sizeof str,"%llx",i);
			if(len<0)throw"snprintf";
			return p(len,str);
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
