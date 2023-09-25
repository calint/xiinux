#pragma once
#include"reply.hpp"
namespace xiinux{class doc{
	size_t len;
	char*buf;
//	const char*lastmod;
public:
	inline doc(const char*data,const char*lastmod=nullptr){//:lastmod(lastmod){
//		printf("new doc %p\n",(void*)this);
		const size_t maxlen=K*M;
		len=strnlen(data,maxlen);
		if(len==maxlen)
			throw"overrun";
		buf=new char[len];
		memcpy(buf,data,len);
	}
	inline~doc(){
//		printf(" * delete doc %p\n",(void*)this);
		delete[]buf;
	}
	inline const char*buffer()const{return buf;}
	inline size_t size_in_bytes()const{return len;}
	inline void to(reply&x)const{x.pk(buf,len);}
};}
