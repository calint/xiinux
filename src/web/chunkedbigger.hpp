#pragma once
#include<memory>
using namespace xiinux;
using namespace std;
namespace web{
	class chunkedbigger final:public widget{
	public:
		void to(reply&r)override{
			unique_ptr<chunky>y(/*takes*/r.reply_chunky());
			y->p("HTTP/1.1 200\r\nTransfer-Encoding:chunked\r\nContent-Type:text/plain;charset=utf-8\r\n\r\n");
			y->send_response_header();

			xprinter&x=*y;
			const size_t buflen=1024*1024;
			char buf[buflen];
			char*p=buf;
			unsigned char ch=0;
			for(size_t i=0;i<buflen;i++){
				*p='a'+ch%26;
				p++;
				ch++;
			}
			x.p(buflen,buf);

			y->finish();
		}
	};
}
