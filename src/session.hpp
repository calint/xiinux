#ifndef session_hpp
#define session_hpp
#include"lut.hpp"
#include"widget.hpp"
namespace xiinux{
	class session{
		char*_id;
		lut<char*>kvp;
		lut<widget*>widgets;
	public:
		inline session(/*takes*/char*session_id):_id(session_id){
			sts.sessions++;
	//		printf(" * new session %s @ %p\n",session_id,(void*)this);
		}
		inline~session(){
			sts.sessions--;
	//		printf(" * delete session %s\n",_id);
			free(_id);
			kvp.delete_content(true);
			widgets.delete_content(true);
		}
		inline const char*id()const{return _id;}
		inline void*operator[](const char*key){return kvp[key];}
		inline void put(char*key,/*takes*/char*data){kvp.put(key,data);}
		inline widget*get_widget(const char*key){return widgets[key];}
		inline void put_widget(char*key,/*takes*/widget*o){widgets.put(key,o);}
	};
}
#endif
