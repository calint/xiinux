#pragma once
#include<ctype.h>
namespace xiinux{class args final{
	const int argc;
	const char**argv;
public:
	#define loop()while(true)
	inline args(const int argc,const char*argv[]):argc{argc},argv{argv}{}
	inline bool hasoption(const char short_name){
		if(argc==1)return false;
		auto vv=argv;
		auto i=argc;
		loop(){
			if(i==1)return false;
			vv++;
			i--;
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
	inline const char*getarg(int n,const char*default_value){
		auto vv=argv;
		auto i=argc;
		loop(){
			if(i==1)return default_value;
			vv++;
			i--;
			auto p=*vv;
			if(*p=='-')continue;
			n--;
			if(n==0)return p;
		}
	}
};}
