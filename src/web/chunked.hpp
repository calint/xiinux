#pragma once
#include<memory>
using namespace xiinux;
using namespace std;
namespace web{
	class chunked final:public widget{
	public:
		void to(reply&r)override{
			unique_ptr<chunky>y(/*takes*/r.reply_chunky());
			y->p("HTTP/1.1 200\r\nTransfer-Encoding:chunked\r\nContent-Type:text/plain;charset=utf-8\r\n\r\n");
			y->send_response_header();

			xprinter&x=*y;
			x.p("chunked response").nl();

			y->finish();
		}
	};
}
