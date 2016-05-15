#ifndef COUNTER_hpp
#define COUNTER_hpp
#include<atomic>
namespace web{
	using namespace xiinux;
	using namespace std;
	class counter:public widget{
	public:
		static atomic_int page_counter;
		int my_counter;
		virtual void to(reply&x)override{
			strb sb;
			my_counter++;
			counter::page_counter++;
			sb.p("my counter ").p(my_counter).nl();
			sb.p("counter for this page ").p(counter::page_counter).nl();
			sb.p("sessions ").p(xiinux::sts.sessions).nl();
			x.http(200,sb.getbuf(),sb.getsize());
		}
	};
	atomic_int counter::page_counter;
}
#endif
