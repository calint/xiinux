#pragma once
namespace web{
	using namespace xiinux;
	class notfound:public widget{
		virtual void to(reply&x)override{
			x.http2(404,"not found");
		}
	};
}