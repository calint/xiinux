#pragma once
#include<cassert>
#include<unistd.h>
namespace xiinux{class span{
protected:
	const char*b{nullptr};
	const char*e{nullptr};
public:
	inline span(){}
	inline span(const span&s):b{s.b},e{s.e}{}
	inline span(const char*begin,const char*end):b{begin},e{end}{assert(end>=begin);}
	inline span&operator=(const span&s){b=s.b;e=s.e;return*this;}
	inline bool operator==(const span&s){return b==s.b and e==s.e;}
	inline const char*begin()const{return b;}
	inline const char*end()const{return e;}
	inline size_t len()const{return e-b;}
	inline bool isempty()const{return b==e;}
	inline span trimleft()const{
		const char*p{b};
		while(true){
			if(p==e)break;
			if(!isspace(*p))break;
			p++;
		}
		return span{p,e};
	}
	inline span&write_to(int fd){
		const size_t ln=len();
		const ssize_t nn=write(fd,b,ln);
		if(nn<0)throw"write";
		if((unsigned)nn!=ln)throw"writeincomplete";
		return*this;
	}
	inline bool startswithstr(const char*s){
		const char*p{b};
		while(true){
			if(!*s)return true;
			if(p==e)return false;
			if(*s!=*p)return false;
			s++;
			p++;
		}
	}
	inline void relocate(const ssize_t diff){if(!b)return;b+=diff;e+=diff;}
};}


//	inline span(const char*buf,const size_t sizeofbuf):bgn{buf},end{buf+sizeofbuf}{}
//	inline span sub(const char*start,const size_t size)const{
//		assert(start>=pt and (start+size)<=(pt+ln));
//		span s=span(start,size);
//		return s;
//	}
