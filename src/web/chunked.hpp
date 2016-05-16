#ifndef chunked_hpp
#define chunked_hpp
#include<memory>
using namespace xiinux;
using namespace std;
namespace web{
	class chunked:public widget{
	public:
		virtual void to(reply&r)override final{
			unique_ptr<chunky>cy(r.reply_chunky());
			cy->p("HTTP/1.1 200\r\nTransfer-Encoding:chunked\r\nContent-Type:text/plain;charset=utf-8\r\n\r\n");
			cy->send_response_header();

			xprinter&x=*cy;
			x.p("chunked response").nl();

			cy->finish();
		}
	};
}
#endif
