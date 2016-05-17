#ifndef chunky_hpp
#define chunky_hpp
#include"defines.hpp"
namespace xiinux{
	class chunky:public xprinter{
		#define loop()while(true)
		size_t length{0};
		char buf[4*1024];
		int sockfd;
		inline size_t io_send(const void*buf,size_t len,bool throw_if_send_not_complete=false){
			sts.writes++;
			const ssize_t n=::send(sockfd,buf,len,MSG_NOSIGNAL);
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
		inline chunky&flush(){
			if(length==0)return*this;
			strb b;
			b.p_hex(length).p(2,"\r\n");
			io_send(b.getbuf(),b.getsize(),true);
			size_t sent_total=0;
			loop(){
				loop(){
					const size_t nsend=length-sent_total;
					const size_t n=io_send(buf+sent_total,nsend,false);
					sent_total+=n;
					if(n==nsend)break;
					//blocking
					perr("would block");
				}
				if(sent_total==length)break;
			}
			io_send("\r\n",sizeof "\r\n"-1,true);
			length=0;
			return*this;
		}
		inline chunky&finish(){
			flush();
			io_send("0\r\n\r\n",sizeof "0\r\n\r\n"-1,true);
			return*this;
		}
		inline chunky&send_response_header(){
			io_send(buf,length,true);
			length=0;
			return*this;
		}
		inline chunky&p(/*copies*/const char*str){
			const size_t n=strnlen(str,sizeof buf+1);//. togetbufferoverrun
			return p(n,str);
		}
		inline chunky&p(const size_t len,/*copies*/const char*str){
			const ssize_t sizeofbuf=sizeof buf;
			const ssize_t bufrem=(sizeofbuf-length);
			ssize_t rem=bufrem-len;
			if(rem>=0){
				strncpy(buf+length,str,len);
				length+=len;
				return*this;
			}
			strncpy(buf+length,str,bufrem);
			length+=bufrem;
			flush();
			const char*s=str+bufrem;
			rem=-rem;
			while(true){
				const ssize_t n=sizeofbuf-length;
				const size_t nn=rem<=n?rem:n;
				strncpy(buf+length,s,nn);
				length+=nn;
				flush();
				rem-=nn;
				if(rem==0)
					return*this;
				s+=nn;
			}
		}
		inline chunky&p(const int i){
			char str[32];
			const size_t n=snprintf(str,sizeof str,"%d",i);
			if(n>sizeof str)throw"snprintf";
			return p(n,str);
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
		inline chunky&p(char ch){
			if(sizeof buf-length==0)flush();
			*(buf+length++)=ch;
			return*this;
		}
		inline chunky&nl(){return p('\n');}
		inline chunky&p(const strb&sb){return p(sb.getsize(),sb.getbuf());}

		// html5
		inline chunky&html5(const char*title=""){
			const char s[]="<!doctype html><script src=/x.js></script><link rel=stylesheet href=/x.css>";
			return p(sizeof s,s)
					.p(sizeof "<title>","<title>").p(title).p(sizeof "</title>","</title>");
		}
		inline chunky&to(FILE*f){
			char fmt[32];
			if(snprintf(fmt,sizeof fmt,"%%%zus",length)<1)throw"err";
			fprintf(f,fmt,buf);
		}
	};
}
#endif
