#ifndef sessions_hpp
#define sessions_hpp
namespace xiinux{
	class sessions{
	private:
		lut<session*>all{1024};
	public:
		inline~sessions(){all.delete_content(false);}
		inline void put(char*sid,session*s,bool allow_overwrite=true){all.put(sid,s,allow_overwrite);}
		inline session*get(const char*sid){return all[sid];}
	};
}
#endif
