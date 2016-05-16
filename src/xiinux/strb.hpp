#ifndef strb_hpp
#define strb_hpp
#include"xprinter.hpp"
#include<string.h>
#include<utility>
namespace xiinux{
	class strb:public xprinter{
		size_t size{0};
		char buf[4096];
	//	strb*nxt{nullptr};
	public:
//		inline strb(strb&&s):size(s.size),buf(std::move(s.buf)){}
//		inline strb&operator=(strb&&other){buf=std::move(other.buf);size=other.size;return*this;}
		inline strb(){}
		inline strb(const char*str){p(str);}
		inline strb&flush(){return*this;}
		inline const char*getbuf()const{return buf;}
		inline size_t getsize()const{return size;}
		inline strb&rst(){size=0;return*this;}
		inline strb&p(/*copies*/const char*str){
			const size_t len=strnlen(str,sizeof buf+1);//. togetbufferoverrun
			const ssize_t rem=sizeof buf-size-len;
			if(rem<0)throw"bufferoverrun";
			strncpy(buf+size,str,len);
			size+=len;
			return*this;
		}
		inline strb&p(const size_t len,/*copies*/const char*str){
			const ssize_t rem=sizeof buf-size-len;
			if(rem<0)throw"bufferoverrun";
			strncpy(buf+size,str,len);
			size+=len;
			return*this;
		}
		inline strb&p(const int i){
			char str[32];
			const int len=snprintf(str,sizeof str,"%d",i);
			if(len<0)throw"snprintf";
			return p(len,str);
	//		const ssize_t rem=sizeof buf-size-len;
	//		if(rem<0)throw"bufferoverrun";
	//		strncpy(buf+size,str,len);
	//		size+=len;
	//		return*this;
		}
		inline strb&p_ptr(const void*ptr){
			char str[32];
			const int len=snprintf(str,sizeof str,"%p",ptr);
			if(len<0)throw"p_ptr:1";
			return p(len,str);
		}
		inline strb&nl(){
			if(sizeof buf-size<1)throw"bufferoverrun2";
			*(buf+size++)='\n';
			return*this;
		}
		inline strb&p(const strb&sb){
			const ssize_t rem=sizeof buf-size-sb.size;
			if(rem<0)throw"bufferoverrun";
			strncpy(buf+size,sb.buf,sb.size);
			size+=sb.size;
			return*this;
		}

		// html5
		inline strb&html5(const char*title=""){
			const char s[]="<!doctype html><script src=/x.js></script><link rel=stylesheet href=/x.css>";
			return p(sizeof s,s)
					.p(sizeof "<title>","<title>").p(title).p(sizeof "</title>","</title>");
		}
	//	inline strb&title(const char*str){return p(sizeof "<title>","<title>").p(sizeof str,str).p(sizeof "</title>","</title>");}
	//	inline strb&textarea(){
	//		return p("<textarea id=_txt style='width:40em height:10em'>").p(s.getsize(),s.getbuf()).p("</textarea>");
	//
	//	}
		inline strb&to(FILE*f){
			char fmt[32];
			if(snprintf(fmt,sizeof fmt,"%%%zus",size)<1)throw"err";
			fprintf(f,fmt,buf);
		}
	};
}
#endif
