//-- generated
#include"xiinux/widget.hpp"
#include"web/notfound.hpp"
#include"web/page.hpp"
#include"web/hello.hpp"
#include"web/counter.hpp"
#include"web/typealine.hpp"
#include"web/chunked.hpp"
#include<string.h>
namespace xiinux{
	static inline widget*widgetget(const char*qs){
		if(!strcmp("hello",qs))return new web::hello();
		if(!strcmp("typealine",qs))return new web::typealine();
		if(!strcmp("counter",qs))return new web::counter();
		if(!strcmp("page",qs))return new web::page(nullptr,nullptr);
		if(!strcmp("chunked",qs))return new web::chunked();
		return new web::notfound();
	}
}
