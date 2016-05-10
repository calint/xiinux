$=function(eid){return document.getElementById(eid);}
function ajax_post(uri,data,on_done){
	console.log("ajax post to "+uri+"\n"+data);
	var req=new XMLHttpRequest();
	req.onreadystatechange=function(){
		console.log("state: "+this.readyState);
		switch(this.readyState){
		case 1:console.log("  OPENED");
			console.log("            status:"+this.status);
			console.log("   response length:"+this.response.length);
			this.setRequestHeader('Content-Type','text/plain;charset=utf-8');
			this.send(data);
			break;
		case 2:console.log("  HEADERS_RECEIVED");
			console.log("            status:"+this.status);
			console.log("   response length:"+this.response.length);
			break;
		case 3:console.log("  LOADING");
			console.log("            status:"+this.status);
			console.log("   response length:"+this.response.length);
			break;
		case 4:// Loaded
			console.log("  DONE");
			console.log("            status:"+this.status);
			console.log("   response length:"+this.response.length);
			console.log("          response:\n"+this.response);
			console.log("\n-------------------------------------------------");
			if(on_done)on_done(this);
			break;
		default:throw"unknownstate";
		}
	};
	req.onerror=function(){
		console.log("error");
	}
	req.ontimeout=function(){
		console.log("timeout");
	}
	req.open('post',uri,true);
}
