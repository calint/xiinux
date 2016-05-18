#ifndef A_hpp
#define A_hpp
#include<typeinfo>
namespace web{
	using namespace xiinux;
	class a:public widget{
		/*ref*/a*pt{nullptr};// parent
		/*own*/const char*nm{nullptr};// name
	public:
		a(/*refs*/a*parent,/*takes*/const char*name):pt(parent),nm(name){
//			printf("%s:%d %s  #    new  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
		}
		virtual~a(){
//			printf("%s:%d %s  # delete  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
			delete nm;
		};
		inline a*getparent()const{return pt;}
		inline void setparent(a*p){pt=p;}
		inline void setname(/*takes*/const char*name){if(nm)delete(nm);nm=name;}
		inline const char*getname()const{return nm;}
		virtual void to(reply&x)override{
			strb s;
	//		s.p(__FILE__).p(":").p(__LINE__).p(" ").p(__PRETTY_FUNCTION__).p("  ");
			s.p(typeid(*this).name()).p("@").p_ptr(this);
			x.http(200,s);
		}
	};
}
#endif
