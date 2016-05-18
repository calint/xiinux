#ifndef HELLO_hpp
#define HELLO_hpp
namespace web{
	using namespace xiinux;
	class hello final:public widget{
		void to(reply&x)override{
			x.http2(200,"hello world");
		}
	};
}
#endif
