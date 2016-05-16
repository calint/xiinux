#ifndef widget_hpp
#define widget_hpp
namespace xiinux{
	class widget{
	public:
		virtual~widget(){
	//		printf(" * delete widget %s  @  %p\n",typeid(*this).name(),(void*)this);
		};
		virtual void to(reply&x)=0;
	//	virtual void ax(reply&x,char*a[]=0){if(a)x.pk(a[0]);}
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len){};
	};
}
#endif
