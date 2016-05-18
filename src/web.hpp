//-- generated
#include"web/hello.hpp"
#include"web/typealine.hpp"
#include"web/counter.hpp"
#include"web/page.hpp"
#include"web/chunked.hpp"
#include"web/chunkedbig.hpp"
#include"web/chunkedbigger.hpp"
#include"web/notfound.hpp"
namespace xiinux{
	static inline widget*widgetget(const char*qs){
		if(!strcmp("hello",qs))return new web::hello();
		if(!strcmp("typealine",qs))return new web::typealine();
		if(!strcmp("counter",qs))return new web::counter();
		if(!strcmp("page",qs))return new web::page(nullptr,nullptr);
		if(!strcmp("chunked",qs))return new web::chunked();
		if(!strcmp("chunkedbig",qs))return new web::chunkedbig();
		if(!strcmp("chunkedbigger",qs))return new web::chunkedbigger();
		return new web::notfound();
	}
}
