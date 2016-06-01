#pragma once
#include"reply.hpp"
namespace xiinux{
	class widget{
	public:
		virtual~widget(){};
		virtual void to(reply&x)=0;
		virtual void on_content(reply&x,/*scan*/const char*buf,const size_t len,const size_t total_content_len){};
	};
}
