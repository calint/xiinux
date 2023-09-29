function dbg(str){console.log(str)}
//function dbg(str){}
$=function(eid){return document.getElementById(eid);}
function ajax_post(uri,data,on_done){
	dbg("ajax post to "+uri+"\n"+data);
	var req=new XMLHttpRequest();
	req.onreadystatechange=function(){
		dbg("state: "+this.readyState);
		switch(this.readyState){
		case XMLHttpRequest.OPENED:
			dbg("  OPENED");
			dbg("            status:"+this.status);
			dbg("   response length:"+this.response.length);
			this.setRequestHeader('Content-Type','text/plain;charset=utf-8');
			this.send(data);
			break;
		case XMLHttpRequest.HEADERS_RECEIVED:
			dbg("  HEADERS_RECEIVED");
			dbg("            status:"+this.status);
			dbg("   response length:"+this.response.length);
			break;
		case XMLHttpRequest.LOADING:
			dbg("  LOADING");
			dbg("            status:"+this.status);
			dbg("   response length:"+this.response.length);
			break;
		case XMLHttpRequest.DONE:
			dbg("  DONE");
			dbg("            status:"+this.status);
			dbg("   response length:"+this.response.length);
			dbg("          response:\n"+this.response);
			dbg("\n-------------------------------------------------");
			if(on_done)on_done(this);
			break;
		default:throw"unknownstate";
		}
	};
	req.onerror=function(){
		dbg("error");
	}
	req.ontimeout=function(){
		dbg("timeout");
	}
	req.open('post',uri,true);
}
