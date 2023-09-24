#pragma once
#include<memory>
using namespace xiinux;
using namespace std;
namespace web{
	class chunkedbig final:public widget{
	public:
		void to(reply&r)override{
			unique_ptr<chunky>y(/*take*/r.reply_chunky());
			y->p("HTTP/1.1 200\r\nTransfer-Encoding:chunked\r\nContent-Type:text/plain;charset=utf-8\r\n\r\n");
			y->send_response_header();//? send_session_id


			xprinter&x=*y;
			for(unsigned i=0;i<4*1024;i++){
				x.p("chunked response ");
			}


			y->finish();
		}
	};
}
