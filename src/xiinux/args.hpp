#ifndef args_hpp
#define args_hpp
#include"defines.hpp"
namespace xiinux{
	class args{
		const int argc;
		const char**argv;
	public:
		args(const int argc,const char*argv[]):argc{argc},argv{argv}{}
		inline bool hasoption(const char short_name){
			auto vv=argv;
			loop(){
				auto i=argc-1;
				if(i==0)return false;
				vv++;
				auto p=*vv;
				if(*p=='-'){
					p++;
					loop(){
						const auto ch=*p;
						if(ch==short_name)return true;
						if(ch==0)break;
						if(isdigit(ch))break;
						p++;
					}
					return false;
				}
			}
		}
		inline const char*getoptionvalue(const char short_name,const char*default_value){
			auto i=argc-1;
			if(i==0)return default_value;
			auto vv=argv;
			loop(){
				vv++;
				auto p=*vv;
				if(*p=='-'){
					p++;
					loop(){
						const auto ch=*p;
						if(!ch)break;
						if(ch==short_name){
							p++;
							if(!*p){//? secondparametervaluestartswith
								if(i>1)return*(vv+1);
								return default_value;
							}
							return p;
						}
						p++;
					}
				}
				i--;
				if(i==0)break;
			}
			return default_value;
		}
	};
}
#endif
