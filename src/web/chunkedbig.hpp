#ifndef chunkedbig_hpp
#define chunkedbig_hpp
#include<memory>
using namespace xiinux;
using namespace std;
namespace web{
	class chunkedbig:public widget{
	public:
		virtual void to(reply&r)override final{
			//auto cy=unique_ptr<chunky*>(r.reply_chunky());
			auto cy=r.reply_chunky();
			cy->p("HTTP/1.1 200\r\nTransfer-Encoding:chunked\r\nContent-Type:text/plain;charset=utf-8\r\n\r\n");
			cy->send_response_header();

			xprinter&x=*cy;
			for(auto i=0;i<4096;i++){
				x.p("chunked response ").nl();
			}
			cy->flush().finish();
			delete cy;
		}
	};
}
#endif
