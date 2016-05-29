#pragma once
#include<stdlib.h>
#include<stdio.h>
namespace xiinux{
	class stats{
	public:
		size_t ms{0};
		size_t socks{0};
		size_t sessions{0};
		size_t requests{0};
		size_t input{0};
		size_t output{0};
		size_t accepts{0};
		size_t reads{0};
		size_t writes{0};
		size_t files{0};
		size_t widgets{0};
		size_t cache{0};
		size_t errors{0};
		size_t brkp{0};
		void printhdr(FILE*f){
			fprintf(f,"%12s%12s%12s%8s%8s%8s%8s%12s%12s%12s%12s%12s%8s%8s\n","ms","input","output","socks","reqs","sess","accepts","reads","writes","files","widgets","cache","errors","brkp");
			fflush(f);
		}
		void print(FILE*f){
			fprintf(f,"\r%12zu%12zu%12zu%8zu%8zu%8zu%8zu%12zu%12zu%12zu%12zu%12zu%8zu%8zu",ms,input,output,socks,requests,sessions,accepts,reads,writes,files,widgets,cache,errors,brkp);
			fflush(f);
		}
	};
	stats sts;
}
