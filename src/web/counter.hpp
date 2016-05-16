#ifndef COUNTER_hpp
#define COUNTER_hpp
#include<atomic>
using namespace xiinux;
using namespace std;
namespace web{
	class counter:public widget{
	public:
		static atomic_int page_counter;
		int my_counter;
		virtual void to(reply&r)override{
			my_counter++;
			counter::page_counter++;
			strb sb;
			xprinter&x=sb;
			x.p("my counter ").p(my_counter).nl();
			x.p("counter for this page ").p(counter::page_counter).nl();
			x.p("sessions ").p((int)sts.sessions).nl();
			r.http(200,sb.getbuf(),sb.getsize());
		}
	};
	atomic_int counter::page_counter;
}
#endif
