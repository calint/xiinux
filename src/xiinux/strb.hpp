#pragma once
#include"xprinter.hpp"
#include<string.h>
namespace xiinux{class strb:public xprinter{
	size_t length{0};
	char buf[4096];
public:
	inline strb(){}
	inline strb(const char*str){p(str);}
	inline strb&flush(){return*this;}
	inline const char*getbuf()const{return buf;}
	inline size_t getsize()const{return length;}
	inline strb&rst(){length=0;return*this;}
	inline strb&p(/*copies*/const char*str){
		const size_t len=strnlen(str,sizeof buf+1);//. togetbufferoverrun
		const ssize_t rem=sizeof buf-length-len;
		if(rem<0)throw"bufferoverrun";
		strncpy(buf+length,str,len);
		length+=len;
		return*this;
	}
	inline strb&p(const size_t len,/*copies*/const char*str){
		const ssize_t rem=sizeof buf-length-len;
		if(rem<0)throw"bufferoverrun";
		strncpy(buf+length,str,len);
		length+=len;
		return*this;
	}
	inline strb&p(const int i){
		char str[32];
		const int len=snprintf(str,sizeof str,"%d",i);
		if(len<0)throw"snprintf";
		return p(len,str);
	}
	inline strb&p(const size_t i){
		char str[32];
		const int len=snprintf(str,sizeof str,"%zu",i);
		if(len<0)throw"snprintf";
		return p(len,str);
	}
	inline strb&p_ptr(const void*ptr){
		char str[32];
		const int len=snprintf(str,sizeof str,"%p",ptr);
		if(len<0)throw"p_ptr:1";
		return p(len,str);
	}
	inline strb&p_hex(const long long i){
		char str[32];
		const int len=snprintf(str,sizeof str,"%llx",i);
		if(len<0)throw"snprintf";
		return p(len,str);
	}
	inline strb&p(char ch){
		if(sizeof buf-length==0)flush();
		*(buf+length++)=ch;
		return*this;
	}
	inline strb&nl(){return p('\n');}
	inline strb&p(const strb&sb){
		const ssize_t rem=sizeof buf-length-sb.length;
		if(rem<0)throw"bufferoverrun";
		strncpy(buf+length,sb.buf,sb.length);
		length+=sb.length;
		return*this;
	}

	// html5
	inline strb&html5(const char*title=""){
		const char s[]="<!doctype html><script src=/x.js></script><link rel=stylesheet href=/x.css>";
		return p(sizeof(s)-1,s) // -1 to not copy the terminator \0
				.p(7,"<title>").p(title).p(8,"</title>"); // 7 and 8 are the number of bytes to copy
	}
	inline strb&to(FILE*f){
		char fmt[32];
		if(snprintf(fmt,sizeof fmt,"%%%zus",length)<1)throw"err";
		fprintf(f,fmt,buf);
	}
};}
