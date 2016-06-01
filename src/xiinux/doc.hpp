#pragma once
#include"reply.hpp"
namespace xiinux{class doc{
	size_t len;
	char*buf;
//	const char*lastmod;
public:
	inline doc(const char*data,const char*lastmod=nullptr){//:lastmod(lastmod){
//		printf("new doc %p\n",(void*)this);
		len=strnlen(data,1024*1024*1024); //? overrun
		buf=(char*)malloc(len);
		memcpy(buf,data,len);
	}
	inline~doc(){
//		printf(" * delete doc %p\n",(void*)this);
		free(buf);
//		delete buf;
	}
	inline const char*buffer()const{return buf;}
	inline size_t size_in_bytes()const{return len;}
	inline void to(reply&x)const{x.pk(buf,len);}
};}
