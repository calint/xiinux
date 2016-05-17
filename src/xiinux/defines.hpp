#ifndef defines_hpp
#define defines_hpp
namespace xiinux{
	static const char*application_name="xiinux web server";
	static const int K=1024;
	static const size_t conbufnn=K;
	static const int nclients=K;

	#define loop()while(true)
	#define perr(str)printf("%s:%d %s   ",__FILE__,__LINE__,__PRETTY_FUNCTION__);perror(str);/*throw"perr";*/
	#define dbg(str)printf("%s:%d %s   %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,str);
//	#define dbg(str)/*nodebug*/
}
#endif
