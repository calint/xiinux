#ifndef chunkedbig_hpp
#define chunkedbig_hpp
#include<memory>
using namespace xiinux;
using namespace std;
namespace web{
	class chunkedbig:public widget{
	public:
		virtual void to(reply&r)override final{
//			unique_ptr<chunky>y(/*takes*/r.reply_chunky());
			chunky*y=/*takes*/r.reply_chunky();
			y->p("HTTP/1.1 200\r\nTransfer-Encoding:chunked\r\nContent-Type:text/plain;charset=utf-8\r\n\r\n");
			y->send_response_header();

//			xprinter&x=*y;
			for(auto i=0;i<1024;i++){
				y->p("chunked response\n");
			}
//			x.flush();
			y->finish();
			delete y;
		}
	};
}
#endif
