#pragma once
#include"lut.hpp"
#include"session.hpp"
namespace xiinux{class sessions{
	lut<session*>all{K};
public:
	inline~sessions(){all.delete_content(false);}
//		inline void put(char*sid,session*s,bool allow_overwrite=true){all.put(sid,s,allow_overwrite);}
	inline void put(/*take*/session*s,bool allow_overwrite=true){all.put((char*)s->id(),s,allow_overwrite);}
	inline session*get(const char*sid){return all[sid];}
}sess;}
