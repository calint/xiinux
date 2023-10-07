ui={}
ui.is_dbg=true;
ui.is_dbg_set=true;
ui.is_dbg_pb=true;
ui.is_dbg_verbose=false;
ui.is_dbg_js=true;

$=function(eid){return document.getElementById(eid);}
$d=function(v){console.log(v);}
$s=function(eid,txt){
	// extract inline script
	const tag_bgn='<script>';
	const tag_bgn_len=tag_bgn.length;
	const tag_end='</script>';
	const tag_end_len=tag_end.length;
	const inlines=[];
	let i=0;
	while(true){
		const i_bgn=txt.indexOf(tag_bgn,i);
		if(i_bgn==-1)
			break;
		const i_end=txt.indexOf(tag_end,i_bgn+tag_bgn_len);
		if(i_end==-1){
			alert('Did not find end tag after index '+(i_bgn+tag_bgn_len));
			break;
		}
		const script=txt.substring(i_bgn+tag_bgn_len,i_end);
		inlines.push(script);
		txt=txt.substring(0,i_bgn)+txt.substring(i_end+tag_end_len,txt.length);
		i=i_bgn;
	}
	// set txt stripped of inline script
	const e=$(eid);
	if(ui.is_dbg_set)$d(eid+'{'+txt+'}');
	if(!e){$d(eid+' notfound');return;}
	if(e.nodeName=='INPUT'||e.nodeName=='TEXTAREA'||e.nodeName=='OUTPUT'){
		e.value=txt;
	}else{
		e.innerHTML=txt;
	}

	for(let i=0;i<inlines.length;i++){
		console.log('inline: '+inlines[i]);
		eval(inlines[i]);
	}
}
$sv=function(eid,txt){
	const e=$(eid);
	if(ui.is_dbg_set)$d(eid+'{'+txt+'}');
	if(!e){$d(eid+' notfound');return;}
	if(e.nodeName=='INPUT'||e.nodeName=='TEXTAREA'||e.nodeName=='OUTPUT'){
		e.value=txt;
		$b(e);
	}else{
		e.innerHTML=txt;
		if(e.contentEditable=='true')
			$b(e);
	}
}
$o=function(eid,txt){
	if(ui.is_dbg_set)$d(eid+'={'+txt+'}');
	const e=$(eid);if(!e)return;e.outerHTML=txt;
}
$p=function(eid,txt){
	const e=$(eid);
	if(e.nodeName=='INPUT'||e.nodeName=='TEXTAREA'||e.nodeName=='OUTPUT'){
		e.value+=txt;
		$b(e);
	}else{
		e.innerHTML+=txt;
		if(e.contentEditable=='true')
			$b(e);
	}
}
$a=function(eid,a,v){$(eid).setAttribute(a,v);}
$r=function(ev,ths,axpb){if(ev.keyCode!=13)return true;$x(axpb);return false;}
$f=function(eid){
	if(ui.focusDone)return;
	const e=$(eid);
	if(!e)return;
	if(e.focus)e.focus();
	if(e.setSelectionRange)e.setSelectionRange(e.value.length,e.value.length);
	/*if(e.select)e.select();*/
	ui.focusDone=true;
}
$t=function(s){document.title=s;}
ui.alert=function(msg){alert(msg);}
ui._clnfldvl=function(s){return s.replace(/\r\n/g,'\n').replace(/\r/g,'\n');}
ui._hash_key=function(ev){
	const kc=(ev.shiftKey?'s':'')+(ev.ctrlKey?'c':'')+(ev.altKey?'a':'')+(ev.metaKey?'m':'')+String.fromCharCode(ev.keyCode);
	$d(kc);
	return kc;
}
ui.scrollToTop=function(){window.scrollTo({top:0,behavior:'smooth'});}
ui.debounceTimeoutId=null;
ui.debounce=function(callback,interval){
	clearTimeout(ui.debounceTimeoutId);
	ui.debounceTimeoutId=setTimeout(()=>{callback()},interval);
}
ui.keys=[];
ui.onkey=function(ev){
	const cmd=ui.keys[ui._hash_key(ev)];
	if(cmd)eval(cmd);
}
ui.fmt_size=function(num){
	return num.toString().replace(/\B(?=(\d{3})+\b)/g,',');
}
ui.fmt_data_per_sec=function(nbytes,ms){
	let b_per_s=Math.floor(nbytes*1024/ms);
	if(b_per_s<1024){
		return b_per_s+' B/s';
	}
	b_per_s>>=10;
	if(b_per_s<1024){
		return b_per_s+' KB/s';
	}
	b_per_s>>=10;
	if(b_per_s<1024){
		return b_per_s+' MB/s';
	}
	b_per_s>>=10;
	if(b_per_s<1024){
		return b_per_s+' GB/s';
	}
	b_per_s>>=10;
	if(b_per_s<1024){
		return b_per_s+' TB/s';
	}
}
ui._pb=[];
ui.pbhas=function(id){return id in ui._pb;}
$b=function(e){
	if(ui.is_dbg_pb)$d('qpb '+e.id);
	if(e.id in ui._pb)return;
	ui._pb[e.id]=e.id;
}

ui._axc=0;
ui._axconwait=false;
ui._onreadystatechange=function(){
//	$d(" * stage "+this.readyState);
	const elsts=$('-ajaxsts');
	if(elsts){
		const e=elsts;
		if(e._oldbg!=null){
			e.style.background=e._oldbg;
			delete e._oldbg;
		}
	}
	switch(this.readyState){
	case 1:{// Open
		$d('req open');
		if(ui.is_dbg_verbose)$d(new Date().getTime()-this._t0+" * sending");
		$s('-ajaxsts','sending '+this._pd.length+' text');
		this.setRequestHeader('Content-Type','text/plain; charset=utf-8');
		$d(this._pd.replace('\r','\n'));
		ui.req._jscodeoffset=0;
		ui.focusDone=false;
		this.send(this._pd);
		break;
	}
	case 2:{// Sent
		$d('req sent');
		const dt=new Date().getTime()-this._t0;
//		$d(dt+" * sending done");
		$s('-ajaxsts','sent '+this._pd.length+' in '+dt+' ms');
		break;
	}
	case 3:{// Receiving
		$d('req receiving');
//		$d(new Date().getTime()-this._t0+" * reply code "+this.status);
		const s=this.responseText.charAt(this.responseText.length-1);
		const ms=new Date().getTime()-this._t0;
		$s('-ajaxsts','received '+ui.fmt_size(this.responseText.length)+' text '+ui.fmt_data_per_sec(this.responseText.length,ms));
//		console.log('receiving '+this.responseText.length+' text');
		if(s!='\n'){
//			$d(new Date().getTime()-this._t0+" * not eol "+(this.responseText.length-this._jscodeoffset));
			break;
		}
		const jscode=this.responseText.substring(this._jscodeoffset);
		if(ui.is_dbg_js)$d(new Date().getTime()-this._t0+" * run "+jscode.length+" bytes");
		if(ui.is_dbg_js)$d(jscode);
		this._jscodeoffset+=jscode.length;
		eval(jscode);
		break;
	}
	case 4:{// Loaded
		$d('req loaded');
		this._pd=null;
		ui._pb=[];

		let jscode=this.responseText.substring(this._jscodeoffset);
		if(jscode.length>0){
			if(ui.is_dbg_js)$d(new Date().getTime()-this._t0+" * run "+jscode.length+" bytes");
			if(ui.is_dbg_js)$d(jscode);
			this._jscodeoffset+=jscode.length;
			eval(jscode);
		}
		this._dt=new Date().getTime()-this._t0;//? var _dt
		$s('-ajaxsts',this._dt+' ms '+ui.fmt_size(this.responseText.length)+' chars '+ui.fmt_data_per_sec(this.responseText.length,this._dt));
		$d("~~~~~~~ ~~~~~~~ ~~~~~~~ ~~~~~~~ ")
//		$d("done in "+this._dt+" ms");
		ui.focusDone=false;
		break;
	}
	default:throw "unknown state";	
	}
}
$x=function(pb){
	ui._axc++;
	$d("\n\nrequest #"+ui._axc);
	let post=pb+'\r';
	for(const id in ui._pb){
		//$d('field '+id);
		const e=$(id)
		post+=e.id+'=';			
		if(e.value!==undefined)
			post+=ui._clnfldvl(e.value);
		else{
			post+=ui._clnfldvl(e.innerHTML);
		}
		post+='\r';
	}
	$d("~~~~~~~ ~~~~~~~ ~~~~~~~ ~~~~~~~ ")
	if(!ui.req){
		ui.req=new XMLHttpRequest();
		ui.req.onreadystatechange=ui._onreadystatechange;
		ui.req.onerror=function(){
			const e=$('-ajaxsts');
			if(!e)return;
			e._oldbg=e.style.background;
			e.style.background='#f00';
			$s('-ajaxsts','connection to server lost. try reload or wait and re-try.');
		}
		$s('-ajaxsts'," * new connection");
	}else{
		$s('-ajaxsts'," * reusing connection");
		let count=0;
		while(ui.req.readyState==1||ui.req.readyState==2||ui.req.readyState==3){
			if(ui._axconwait){
				$d("  * busy, waiting");
				alert("connection busy. waiting.");
				count++;
				if(count>3)
					throw "waiting cancelled";
			}else{
				$d("   * busy, cancelling");
				ui.req.abort();
			}
		}	
	}
	ui.req._t0=new Date().getTime();
	ui.req._pd=post;
	ui.req.open('post',location.href,true);
}
