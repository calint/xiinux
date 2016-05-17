#ifndef widget_hpp
#define widget_hpp
#include"reply.hpp"
namespace xiinux{
	class widget{
	public:
		virtual~widget(){};
		virtual void to(reply&x)=0;
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len){};
	};
}
#endif
