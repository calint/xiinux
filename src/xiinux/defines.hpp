#ifndef defines_hpp
#define defines_hpp
namespace xiinux{
	#define APP "xiinux web server"
	static int const K=1024;
	static size_t const conbufnn=K;
	static int const nclients=K;

	#define loop()while(true)
	#define perr(str)printf("%s:%d %s  ",__FILE__,__LINE__,__PRETTY_FUNCTION__);perror(str);
	#define dbg(str)printf("%s:%d %s   %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,str);
//	#define dbg(str)
}
#endif
