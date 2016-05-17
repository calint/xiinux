#ifndef xprinter_hpp
#define xprinter_hpp
namespace xiinux{
	class xprinter{
	public:
		virtual~xprinter(){};
		virtual inline xprinter&p(/*scan*/const char*str)=0;
		virtual inline xprinter&p(const size_t len,/*scan*/const char*str)=0;
		virtual inline xprinter&p(const int nbr)=0;
		virtual inline xprinter&p(const size_t nbr)=0;
		virtual inline xprinter&p_ptr(const void*ptr)=0;
		virtual inline xprinter&p_hex(const long long nbr)=0;
		virtual inline xprinter&p(char ch)=0;
		virtual inline xprinter&nl()=0;
		virtual inline xprinter&html5(const char*title="")=0;
		virtual inline xprinter&flush()=0;
	};
}
#endif
