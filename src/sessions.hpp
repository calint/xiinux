#ifndef sessions_hpp
#define sessions_hpp
#include"lut.hpp"
#include"session.hpp"
namespace xiinux{
	class sessions{
	public:
		inline~sessions(){
	//		printf(" * delete sessions %p\n",(void*)this);
			all.delete_content(false);
		}
		lut<session*>all{1024};
	};
}
#endif
