#pragma once
#include<stdio.h>
namespace xiinux{
	static constexpr const char*application_name="xiinux web server";
	static constexpr int K=1024;
	static constexpr int M=K*K;
	static constexpr int nclients=128;
	static constexpr size_t sock_req_buf_size_in_bytes=K;
	static constexpr size_t sock_content_buf_size_in_bytes=4*K;
	static constexpr size_t chunky_buf_size_in_bytes=4*K;
	static constexpr const char*signal_connection_reset_by_peer="brk";

	#define perr(str)do{printf("%s:%d %s   ",__FILE__,__LINE__,__PRETTY_FUNCTION__);perror(str);/*throw"perr";*/}while(0);
	#define dbg(str)do{printf("%s:%d %s   %s\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,str);}while(0);
}
