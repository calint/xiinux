#pragma once
namespace web{
	using namespace xiinux;
	class typealine:public widget{
		void to(reply&x)override{
			x.http2(200,"typealine");
		}
		void on_content(reply&x,/*scan*/const char*buf,const size_t len,const size_t total_len)override{
			if(xiinux::conf::print_trafic)write(1,buf,len);
			if(len==total_len){
				x.http(200,buf,len);
				return;
			}
			if(len==0){// init content scan
				char s[K];
				const size_t n=snprintf(s,sizeof s,"HTTP/1.1 200\r\nContent-Length: %zu\r\n\r\n",total_len);
				x.pk(s,n);
				return;
			}
			x.pk(buf,len);
		}
	};
}
