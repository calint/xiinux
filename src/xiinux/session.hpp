#pragma once
#include"widget.hpp"
namespace xiinux{class session{
	char*key;
	lut<char*>kvp;
	lut<widget*>widgets;
public:
	inline session(/*takes*/char*session_id):key{session_id}{sts.sessions++;}
	inline~session(){
		sts.sessions--;
		free(key);
		kvp.delete_content(true);
		widgets.delete_content(true);
	}
	inline const char*id()const{return key;}
	inline void*operator[](const char*key){return kvp[key];}
	inline void put(char*key,/*takes*/char*data){kvp.put(key,data);}
	inline widget*get_widget(const char*key){return widgets[key];}
	inline void put_widget(char*key,/*takes*/widget*o){widgets.put(key,o);}
};}
