#ifndef WEB_hpp
#define WEB_hpp
#include"xiinux.hpp"
//-- application
namespace web{
	using namespace xiinux;
	class hello:public widget{
		virtual void to(reply&x)override{
			x.http2(200,"hello world");
		}
	};
	class bye:public widget{
		virtual void to(reply&x)override{
			x.http2(200,"b y e");
		}
	};
	class notfound:public widget{
		virtual void to(reply&x)override{
			x.http2(404,"path not found");
		}
	};
	class typealine:public widget{
		virtual void to(reply&x)override{
			x.http2(200,"typealine");
		}
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
	//		printf(" typealine received content: %s\n",content);
			x.http(200,content,content_len);
		}
	};
	class counter:public widget{
		int c;
		virtual void to(reply&x)override{
			strb sb;sb.p("counter ").p(++c).nl();
	//		strb().p("counter ").p(++c).nl();
			x.http(200,sb.getbuf(),sb.getsize());
		}
	};
	class notes:public widget{
		strb txt;
	public:
		notes(){
			txt.p("notebook\n");
		}
		virtual void to(reply&x)override{
			strb s;
			s.html5("notebook")
					.p("<input id=_btn type=button value=save onclick=\"this.disabled=true;ajax_post('/?notes',$('_txt').value,function(r){console.log(r);$('_btn').disabled=false;eval(r.responseText);})\"><br>\n")
					.p("<textarea id=_txt class=big>")
					.p(txt)
					.p("</textarea>")
					.p("<script>$('_txt').focus()</script>")
					.nl();
			x.http(200,s.getbuf(),s.getsize());
		}
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
			txt.
				rst().
				p(content_len,content);
			x.http2(200,"location.reload();");
		}
	};
	class a:public widget{
		/*ref*/a*pt{nullptr};// parent
		/*own*/const char*nm{nullptr};// name
	public:
		a(/*refs*/a*parent,/*takes*/const char*name):pt(parent),nm(name){
			printf("%s:%d %s  #    new  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
		}
		virtual~a(){
			printf("%s:%d %s  # delete  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
			delete nm;
		};
		inline a*getparent()const{return pt;}
		inline void setparent(a*p){pt=p;}
		inline void setname(/*takes*/const char*name){if(nm)delete(nm);nm=name;}
		inline const char*getname()const{return nm;}
		virtual void to(reply&x){
			strb s;
	//		s.p(__FILE__).p(":").p(__LINE__).p(" ").p(__PRETTY_FUNCTION__).p("  ");
			s.p(typeid(*this).name()).p("@").p_ptr(this);
			x.http(200,s);
		}
	};
	class an:public a{
	public:
		const char*id;
		virtual~an(){
			printf("%s:%d %s  # delete  %s  @  %p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
			delete id;
		};
		virtual void to(reply&x)=0;
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len){};
	};

	class page:public a{
		strb txt;
	//	lst<page*>sub;
	public:
		page(a*parent,/*takes*/const char*name):a(parent,name){
			printf("%s:%d %s  # new page  %s@%p\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,typeid(*this).name(),(void*)this);
		}
		virtual void to(reply&x)override{
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
		virtual void on_content(reply&x,/*scan*/const char*content,const size_t content_len)override{
			txt.
			rst().
				p(content_len,content);
			x.http2(200,"location.reload();");
		}
	};
}//namespace web
#endif
