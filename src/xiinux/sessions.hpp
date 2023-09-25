#pragma once
#include"lut.hpp"
#include"session.hpp"
namespace xiinux{class sessions final{
	lut<session*>all_{K};
public:
	inline~sessions(){all_.delete_content(false);}
//		inline void put(char*sid,session*s,bool allow_overwrite=true){all.put(sid,s,allow_overwrite);}
	inline void put(/*take*/session*s,bool allow_overwrite=true){all_.put(s->id(),s,allow_overwrite);}
	inline session*get(const char*sid){return all_[sid];}
}static sess;}
