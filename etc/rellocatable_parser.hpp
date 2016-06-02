#pragma once
class relocatable{
		virtual void relocatable_relocate(const ssize_t diff)=0;
	};

	class parser{
	public:
		enum ioop{read,done};
		virtual void parser_init(const char*buf,const char*eob)=0;
		virtual ioop parser_parse()=0;
		virtual void parser_ince(size_t n)=0;
		virtual const char*parser_pos()const=0;
	};

	class headers:public relocatable,public parser{
		const char*p{nullptr};
		const char*e{p};
		const char*c{p};
		enum{key,value}st{key};
		class:relocatable{
			span keys[32];
			span values[32];
			size_t n{0};
		public:
			inline void add_key(const char*b,const char*e){keys[n]=span{b,e};}
			inline void add_value(const char*b,const char*e){values[n++]=span{b,e};}
			inline void rst(){n=0;}

			// relocatable
			inline virtual void relocatable_relocate(const ssize_t diff)override{
				for(size_t i=0;i<=n;i++){
					keys[i].relocate(diff);
					values[i].relocate(diff);
				}
			}

		}headers;
	public:
		inline const char*get_method()const{return nullptr;}
		inline const char*get_uri()const{return nullptr;}
		inline const char*get_query()const{return nullptr;}
		inline const char*get_section()const{return nullptr;}
		inline const char*get_header_value(const char*key)const{return nullptr;}
		// parser
		inline void parser_init(const char*buf,const char*eob){p=c=buf;e=eob;headers.rst();}
		inline void parser_ince(size_t n)override{e+=n;}
		inline const char*parser_pos()const override{return p;}
		// relocatable
		inline void relocatable_relocate(const ssize_t diff)override{
			p+=diff;
			e+=diff;
			c+=diff;

		}
		inline ioop parser_parse(){loop(){
			if(st==key){
				while(p<e){
					const char ch=*p++;
					if(ch==':'){
						headers.add_key(c,p-1);
						c=p;
						st=value;
						break;
					}
					if(ch=='\n'){
						return done;
					}
				}
				if(st==key){return read;}//? relocate
			}
			if(st==value){
				while(p<e){
					const char ch=*p++;
					if(ch=='\n'){
						headers.add_value(c,p-1);
						c=p;
						st=key;
						break;
					}
				}
				if(p==e){return read;}//? relocate
			}
		}}
	};

	class reqline:public relocatable,public parser{
		const char*b{nullptr};
		const char*p{b};
		const char*e{b};
		const char*c{b};
		enum{method,path,query,pound,protocol}st{method};
		struct{
			span method,path,query,pound,protocol;
		}spans;
	public:
		inline const char*get_method()const{return nullptr;}
		inline const char*get_uri()const{return nullptr;}
		inline const char*get_query()const{return nullptr;}
		inline const char*get_header_value(const char*key)const{return nullptr;}
		// parser
		inline void parser_init(const char*buf,const char*eob){p=c=buf;e=eob;}
		inline void parser_ince(const size_t n)override{e+=n;}
		inline virtual const char*parser_pos()const override{return p;}
		// relocatable
		inline void relocatable_relocate(const ssize_t diff)override{
//			assert(dest+e-b<b);// no overlap
			assert(diff<-16);// some overlap
			memcpy((void*)(b+diff),b,e-b);
			b+=diff;
			e+=diff;
			c+=diff;
			spans.method.relocate(diff);
			spans.path.relocate(diff);
			spans.query.relocate(diff);
			spans.pound.relocate(diff);
			spans.protocol.relocate(diff);
		}
		inline ioop parser_parse(){loop(){
			if(st==method){
				while(p<e){
					const char ch=*p++;
					if(ch==' '){
						c=p;
						st=path;
						break;
					}
					if(ch=='\n'){
						return done;
					}
				}
				if(st==method){return read;}//? relocate
			}
			if(st==path){
				while(p<e){
					const char ch=*p++;
					if(ch=='?'){
						c=p;
						st=query;
						break;
					}
					if(ch=='#'){
						c=p;
						st=pound;
						break;
					}
					if(ch==' '){
						c=p;
						st=protocol;
						break;
					}
				}
				if(st==path){return read;}//? relocate
			}
			if(st==query){
				while(p<e){
					const char ch=*p++;
					if(ch=='#'){
						c=p;
						st=pound;
						break;
					}
					if(ch==' '){
						c=p;
						st=protocol;
						break;
					}
				}
				if(st==query){return read;}//? relocate
			}
			if(st==pound){
				while(p<e){
					const char ch=*p++;
					if(ch==' '){
						c=p;
						st=protocol;
						break;
					}
				}
				if(st==pound){return read;}//? relocate
			}
			if(st==protocol){
				while(p<e){
					const char ch=*p++;
					if(ch=='\n'){
						c=p;
						return done;
					}
				}
				if(st==protocol){return read;}//? relocate
			}
		}}
	};

	class relocatable_parser:public parser,public relocatable{};

	class reqhdrparser:public relocatable,public parser{
		reqline rp;
		headers hp;
		parser&cp{rp};
		enum{parsing_request_line,parsing_headers}st{parsing_request_line};
		const char*b{nullptr};
		const char*e{b};
	public:
		// relocatable
		inline void relocatable_relocate(const ssize_t diff)override{rp.relocatable_relocate(diff);hp.relocatable_relocate(diff);}
		// parser
		inline void parser_init(const char*sob,const char*eob){b=sob;e=eob;cp.parser_init(sob,eob);}
		inline void parser_ince(const size_t n)override{e+=n;cp.parser_ince(n);}
		inline const char*parser_pos()const{return e;}
		inline ioop parser_parse(){loop(){
			switch(cp.parser_parse()){default:throw;case read:return read;case done:
				if(&cp==&rp){
					cp=hp;
					cp.parser_init(rp.parser_pos(),e);
					break;
				}
				// done
				return done;
			}
		}}
	};

	class:public parser{
		enum{parse_request_header,parse_content}st{parse_request_header};
		reqhdrparser rp;
		parser&cp{rp};
//		contentparser cp;

	public:
		inline void parser_init(const char*sob,const char*eob){cp.parser_init(sob,eob);}
		inline void parser_ince(const size_t n)override{cp.parser_ince(n);}
		inline const char*parser_pos()const{return cp.parser_pos();}
		inline ioop parser_parse(){loop(){
			switch(cp.parser_parse()){default:throw;case read:return read;
			case done:
				switch(st){default:throw;
				case parse_request_header:
					cp=rp;
					// content
					// upload
					// file
					// widget
					// cache
					st=parse_content;
				break;
				case parse_content:
					return done;
				}
				break;
			}
		}}
	}reqparser;

	parser&current_parser{reqparser};
