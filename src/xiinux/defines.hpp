#pragma once
#include<stdio.h>
namespace xiinux{
	static const char*application_name="xiinux web server";
	static const int K=1024;
	static const int nclients=K;
	static const size_t sockbuf_size_in_bytes=K;
	static const size_t chunky_buf_size_in_bytes=4*K;
	static const char*signal_connection_reset_by_peer="brk";

	#define loop()while(true)
	#define perr(str)printf("%s:%d %s   ",__FILE__,__LINE__,__PRETTY_FUNCTION__);perror(str);/*throw"perr";*/
	#define dbg(str)printf("%s:%d %s   %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,str);
//	#define dbg(str)/*nodebug*/
}
