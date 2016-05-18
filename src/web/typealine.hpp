#ifndef TYPEALINE_hpp
#define TYPEALINE_hpp
namespace web{
	using namespace xiinux;
	class typealine:public widget{
		void to(reply&x)override{
			x.http2(200,"typealine");
		}
		void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
	//		printf(" typealine received content: %s\n",content);
			x.http(200,content,content_len);
		}
	};
}
#endif
