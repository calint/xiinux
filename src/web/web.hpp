#pragma once
//-- generated
#include"hello.hpp"
#include"typealine.hpp"
#include"counter.hpp"
#include"page.hpp"
#include"chunked.hpp"
#include"chunkedbig.hpp"
#include"chunkedbigger.hpp"
#include"notfound.hpp"
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
