#ifndef COUNTER_hpp
#define COUNTER_hpp
namespace web{
	using namespace xiinux;
	class counter:public widget{
		int c;
		virtual void to(reply&x)override{
			strb sb;sb.p("counter ").p(++c).nl();
	//		strb().p("counter ").p(++c).nl();
			x.http(200,sb.getbuf(),sb.getsize());
		}
	};
}
#endif
