#ifndef AN_hpp
#define AN_hpp
namespace web{
	class an:public a{
	public:
		const char*id;
		virtual~an(){
//			printf("%s:%d %s  # delete  %s  @  %p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
			delete id;
		};
//		virtual void to(reply&x)override=0;
//		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{};
	};
}
#endif
