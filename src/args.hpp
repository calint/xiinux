#ifndef args_hpp
#define args_hpp
namespace xiinux{
	#define loop()while(true)
	class args{
		const int c;
		const char**v;
	public:
		args(const int argc,const char*argv[]):c(argc),v(argv){}
		inline bool hasoption(const char short_name){
			auto vv=v;
			loop(){
				auto i=c-1;
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
			auto i=c-1;
			if(i==0)return default_value;
			auto vv=v;
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
