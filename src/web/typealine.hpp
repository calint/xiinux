#pragma once
namespace web{
	using namespace xiinux;
	class typealine:public widget{
		void to(reply&x)override{
			x.http2(200,"typealine");
		}
		void on_content(reply&x,/*scan*/const char*buf,const size_t len,const size_t total_len)override{
			if(!buf){// init content scan
				char s[K];
				const int n=snprintf(s,sizeof(s),"HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n",total_len);
				x.pk(s,size_t(n));
				return;
			}
			x.pk(buf,len);
		}
	};
}
