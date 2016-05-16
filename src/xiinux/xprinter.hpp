#ifndef xprinter_hpp
#define xprinter_hpp
namespace xiinux{
	class xprinter{
	public:
		virtual~xprinter(){};
		virtual xprinter&p(/*scan*/const char*str)=0;
		virtual xprinter&p(const size_t len,/*scan*/const char*str)=0;
		virtual xprinter&p(const int i)=0;
		virtual xprinter&p_ptr(const void*ptr)=0;
		virtual xprinter&nl()=0;
	//	virtual xprinter&p(const strb&sb)=0;
		virtual xprinter&html5(const char*title="")=0;
		virtual xprinter&flush()=0;
	};
}
#endif
