#ifndef doc_hpp
#define doc_hpp
#include"reply.hpp"
namespace xiinux{
	class doc{
		size_t size;
		char*buf;
	//	const char*lastmod;
	public:
		inline doc(const char*data,const char*lastmod=nullptr){//:lastmod(lastmod){
	//		printf("new doc %p\n",(void*)this);
			size=strlen(data); //? overrun
			buf=(char*)malloc(size);
			memcpy(buf,data,size);
		}
		inline~doc(){
	//		printf(" * delete doc %p\n",(void*)this);
			free(buf);
	//		delete buf;
		}
		inline const char*getbuf()const{return buf;}
		inline size_t getsize()const{return size;}
		inline void to(reply&x)const{x.pk(buf,size);}
	};
}
#endif
