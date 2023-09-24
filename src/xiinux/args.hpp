#pragma once
#include<ctype.h>
namespace xiinux{class args final{
	const int argc;
	const char**argv;
public:
	inline args(const int argc,const char*argv[]):argc{argc},argv{argv}{}
	inline bool hasoption(const char short_name){
		if(argc==1)return false;
		const char**vv=argv;
		int i=argc;
		while(true){
			if(i==1)return false;
			vv++;
			i--;
			const char*p=*vv;
			if(*p=='-'){
				p++;
				while(true){
					const char ch=*p;
					if(ch==short_name)return true;
					if(ch==0)return false;
					if(isdigit(ch))return false;
					p++;
				}
			}
		}
	}
	inline const char*getoptionvalue(const char short_name,const char*default_value){
		int i=argc-1;
		if(i==0)return default_value;
		const char**vv=argv;
		while(true){
			vv++;
			const char*p=*vv;
			if(*p=='-'){
				p++;
				while(true){
					const char ch=*p;
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
		const char**vv=argv;
		int i=argc;
		while(true){
			if(i==1)return default_value;
			vv++;
			i--;
			const char*p=*vv;
			if(*p=='-')continue;
			n--;
			if(n==0)return p;
		}
	}
};}
