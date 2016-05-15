//-- generated
#include "web.hpp"

namespace xiinux{
	static inline widget*widgetget(const char*qs){
		if(!strcmp("hello",qs))return new web::hello();
		if(!strcmp("typealine",qs))return new web::typealine();
		if(!strcmp("bye",qs))return new web::bye();
		if(!strcmp("counter",qs))return new web::counter();
		if(!strcmp("notes",qs))return new web::notes();
		if(!strcmp("page",qs))return new web::page(nullptr,nullptr);
		return new web::notfound();
	}
}

int main(int c,char**a){
	return xiinux::main(c,a);
}
