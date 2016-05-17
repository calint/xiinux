#ifndef PAGE_hpp
#define PAGE_hpp
#include"a.hpp"
namespace web{
	using namespace xiinux;
	class page final:public a{
		strb txt;
	//	lst<page*>sub;
	public:
		page(a*parent=nullptr,/*takes*/const char*name=nullptr):a{parent,name}{
			printf("%s:%d %s  # new page  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
		}
		void to(reply&x)override{
			strb s;
			s.html5("page")
					.p("<input id=_btn type=button value=update onclick=\"this.disabled=true;ajax_post('/?page',$('_txt').value,function(r){console.log(r);$('_btn').disabled=false;eval(r.responseText);})\">")
					.p("\n")
					.p("<textarea id=_txt class=big>")
					.p(txt)
					.p("</textarea>")
					.p("<script>$('_txt').focus()</script>")
					.nl();
			x.http(200,s);
		}
		void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
			txt.
			rst().
				p(content_len,content);
			x.http2(200,"location.reload();");
		}
	};
}
#endif
