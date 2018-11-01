/**********************************
* KM-BASIC web written by Katsumi *
*     This script is released     *
*       under the LGPL v2.1.      *
**********************************/

/*
	restrictions:
	Use (A/B) instead of A/B for parseInt.
	Use only one GOSUB() function in a statement.
	Follow KM-BASIC-MIPS syntax.
	PEEK() is the same as TVRAM(). TVRAM() returns 0 without arguments.
	Execution speed is slower than KM-BASIC for MIPS
	return value from INPUT$() must be inserted to a variable. DO NOT print directly.

*/

basic=new Object();
basic.STRINGS=Array();
basic.source="END";
basic.binary=Array();
basic.compile=function(str){
	var lines=str.split(/\r\n|\r|\n/);
	var i,j;
	var codes;
	this.binary=Array();
	this.STRINGS=Array();
	this.linenum=0;
	for(i=0;i<lines.length;i++){
		try {
			this.binary=this.binary.concat(this.compileLine(lines[i]));
		} catch(e) {
			alert(e+"\nAt line "+(i+1)+": '"+lines[i]+"'");
			this.binary=[];
			break;
		}
	}
	this.binary=this.binary.concat([{"command":"END","param":""}]);
	dom.getElement("code").value=JSON.stringify(this.binary);
};
basic.compileLine=function(str){
	var codes;
	var commands;
	var i,j,m;
	this.linenum++;
	// Trim
	m=str.match(/^[\s]*(.*)[\s]*$/);
	if (m) {
		str=m[1];
	}
	// Detect line number
	m=str.match(/^([0-9]+)[\s](.*)$/);
	if (m) {
		this.linenum=parseInt(m[1],10);
		str=m[2];
	}
	codes=[{"command": "LINE", "param": this.linenum}];
	// Replace strings to "STRINGS[0]" etc.
	var callback=function(m0,m1){
		var i=basic.STRINGS.length;
		basic.STRINGS[i]=m1;
		return "STRINGS["+i.toString(10)+"]";
	};
	str=str.replace(/"(.*?)"/g,callback);
	// Remove REM
	str=str.replace(/(^|[\s])REM.*$/i,"");
	// Split to commands
	commands=str.split(':');
	for(i=0;i<commands.length;i++){
		codes=codes.concat(this.compileCommand(commands[i]));
	}
	return codes;
};
basic.getvalue=function(str){
	// Pickup code for a value.
	// End of string must be ')' ',' ';' ':'
	// return [value-code, remaining-code]
	var i;
	var depth=0;
	for(i=0;i<str.length;i++){
		switch(str.charAt(i)){
			case ')': // case ']':
				if (depth) {
					depth--;
					continue;
				}
				return [str.substr(0,i),str.substr(i)];
			case ',': case ';': case ':':
				if (depth) {
					continue;
				}
				return [str.substr(0,i),str.substr(i)];
			case '(': // case '[':
				depth++;
				continue;
			default:
				continue;
		}
	}
	return [str.substr(0,i),str.substr(i)];
};
basic.todim=function(str1,str2,indim){
	// Change A(...) to A[...] etc.
	if (!indim) indim=0;
	var i=str2.indexOf("(");
	var j=str2.indexOf(")");
	if (0<=j && (i<0 || j<i)) {
		// ")" found
		str1+=str2.substr(0,j);
		str2=str2.substr(j+1);
		if (indim&1) {
			// This was a dimension
			str1+="]";
		} else {
			// This was not dimension
			str1+=")";
		}
		return arguments.callee(str1,str2,indim>>1);
	}
	if (0<=i) {
		// "(" found
		str1+=str2.substr(0,i);
		m=str2.match(/^[^\(]*?([A-Z_]*)\(/);
		if (m[1].length==1) {
			// This is a dimension
			str1+="[";
			return arguments.callee(str1,str2.substr(i+1),(indim<<1)|1);
		} else {
			// This is not dimension
			str1+="(";
			return arguments.callee(str1,str2.substr(i+1),indim<<1);
		}
	} else {
		// Either "(" or ")" not found.
		return str1+str2;
	}
};
basic.compileCommand=function(command){
	var m,codes;
	command=command.toUpperCase();
	// Trim
	command=command.replace(/^[\s]?|[\s]?$/g,"");
	// Replace all spacings to simple " "
	command=command.replace(/[\s]+/g," ");
	// "A ELSE B" -> "A","ELSE","B"
	m=command.match(/^(.*)ELSE (.*$)$/);
	if (m) {
		codes=this.compileCommand(m[1]);
		codes=codes.concat(this.compileCommand("ELSE"));
		codes=codes.concat(this.compileCommand(m[2]));
		return codes;
	}
	// Detect GOSUB().
	m=command.match(/^(.*)GOSUB\((.*)$/);
	if (m) {
		command=m[1];
		m=basic.getvalue(m[2]);
		codes=this.compileCommand("GOSUB "+m[0]);
		codes=codes.concat(this.compileCommand(command+"GOSUBFUNC("+m[1]));
		return codes;
	}
	// Simplify "="
	command=command.replace(/[\s]?=[\s]?/g,"=");
	// Change A(...) to A[...] etc.
	command=this.todim("",command);
	// Replace "A$(" to "FUNC_A$("
	command=command.replace(/([^A-Z])([A-Z])[\$]\(/g,"$1SUBSTR($2,");
	// Replace "A$" to "A"
	command=command.replace(/([A-Z])[\$]/g,"$1");
	// Replace "$" to "0x"
	command=command.replace(/\$([0-9A-F])/g,"0x$1");
	// Return if null
	if (command=="") return [];
	// "=" -> "=="
	while (command.match(/([^=<>])=([^=])/)) command=command.replace(/([^=<>])=([^=])/g,"$1==$2");
	// "<>" -> "!="
	command=command.replace(/<>/g,"!=");
	// "(...)" -> "INT(...)"
	command=command.replace(/([^A-Z])\(/g,"$1INT(");
	// " AND " -> " & "
	command=command.replace(/ AND /g," & ");
	// " OR " -> " | "
	command=command.replace(/ OR /g," | ");
	// " XOR " -> " ^ "
	command=command.replace(/ XOR /g," ^ ");
	// Remove "LET "
	command=command.replace(/^LET /,"");
	// Detect LET (note that "=" has been changed to "==")
	m=command.match(/^([A-Z])==(.*)$/);
	if (m) {
		return [{"command": "LET", "param": m[1]+"=INT("+m[2]+")"}];
	}
	// Detect LET for dimension
	m=command.match(/^([A-Z]\[.*\])==(.*)/);
	if (m) {
		return [{"command": "LET", "param": m[1]+"=INT("+m[2]+")"}];
	}
	// Other commands
	m=command.match(/^([A-Z]+)[\s]*(.*)$/);
	if (!m) {
		alert("Syntax Error: "+command);
		return [];
	}
	command=m[2];
	switch(m[1]){
		case "FOR":
			m=command.match(/^([A-Z])==(.*) TO (.*) STEP (.*)$/);
			if (!m) {
				m=command.match(/^([A-Z])==(.*) TO (.*)$/);
				m[4]="1";
			}
			if (!m) {
				alert("Syntax Error: FOR");
				return [];
			}
			return [
				{"command": "LET", "param": m[1]+"=INT("+m[2]+")"},
				{"command": "FOR", "param": m[1]+"=="+m[3]},
				{"command": "LET", "param": m[1]+"+=("+m[4]+")"}
				];
		case "IF":
			m=command.match(/^(.*) THEN (.*)$/);
			if (!m) {
				alert("Synax Error: IF");
				return [];
			}
			codes=[{"command": "IF", "param": m[1]}];
			try {
				return codes.concat(this.compileCommand(m[2]));
			} catch(e) {
				return codes.concat([{"command": "GOTO", "param": m[2]}]);
			}
		case "ELSE":
			codes=[{"command": "ELSE", "param": ""}];
			try {
				return codes.concat(this.compileCommand(m[2]));
			} catch(e) {
				return codes.concat([{"command": "GOTO", "param": m[2]}]);
			}
		case "CLEAR":
		case "DIM":
		case "PRINT":
		case "POKE":
		case "NEXT":
		case "GOTO":
		case "GOSUB":
		case "RETURN":
		case "END":
		case "CLS":
		case "SOUND":
		case "MUSIC":
		case "RESTORE":
		case "DATA":
		case "CURSOR":
		case "COLOR":
		case "PALETTE":
		case "BGCOLOR":
		case "DRAWCOUNT":
		case "WAIT":
		case "SCROLL":
		case "USEPCG":
		case "PCG":
		case "DEBUG":
			return [{"command": m[1], "param": m[2]}];
		case "LABEL":
			if (!m[2].match(/^[A-Z][A-Z0-9]{0,5}$/)) {
				throw "Wrong label: "+m[2];
				return [];
			}
			return [{"command": m[1], "param": m[2]}];
		default:
			//alert("Syntax Error: "+m[1]);
			throw "Syntax Error: "+m[1];
			return [];
	}
};
basic.run=function(){
	// Prepare local environment for executing code.
	var STRINGS=this.STRINGS;
	var A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z;
	var CLEAR=function(){
		A=B=C=D=E=F=G=H=I=0;
		J=K=L=M=N=O=P=Q=R=0;
		S=T=U=V=W=X=Y=Z=0;
	};
	CLEAR();
	// Initilize program counter etc.
	var pc=0;
	var currentline=0
	var currentlabel="";
	var stack=Array();
	var sp=0;
	var ifexec=1;
	var returnval=0;
	var dataline=-1;
	var datapos=-1;
	var sleep=0;
	var refreshfont=0;
	display.resetpcg();
	// Local vars used in statements and functions
	var runenv=new Object();
	var binary=this.binary;
	// C definitions
	var WIDTH_X=30;
	var WIDTH_Y=27;
	// Declare statements
	runenv.CLEAR=function(){
		A=B=C=D=E=F=G=H=I=0;
		J=K=L=M=N=O=P=Q=R=0;
		S=T=U=V=W=X=Y=Z=0;
	};
	runenv.CLEAR();
	runenv.END=function(){
		pc=binary.length;
	};
	runenv.error=function(str){
		if (currentline) {
			alert("Error at line "+currentline+": "+str);
		} else {
			alert("Error after label "+currentlabel+": "+str);
		}
		this.END();
	};
	runenv.LINE=function(param){
		currentline=param;
		currentlabel="";
		ifexec=1;
	};
	runenv.LABEL=function(param){
		currentline=0;
		currentlabel=param;
		ifexec=1;
	};
	runenv.PRINT=function(param){
		var m,str;
		var morethanonce=0;
		while(param.length){
			m=basic.getvalue(param);
			if (!m) break;
			str=this.eval(m[0]);
			str=INT(str);
			if (!isNaN(str)) str=str.toString(10);
			display.printstr(str);
			morethanonce=1;
			switch(m[1].charAt(0)){
				case ',':
					display.printcomma();
					break;
				case ';':
					break;
				default:
					display.printchar(0x0d);
					break;
			}
			param=m[1].substr(1);
		}
		if (!morethanonce) display.printchar(0x0d);
	};
	runenv.LET=function(param){
		this.eval(param);
	};
	runenv.DIM=function(param){
		var callback=function(m0,m1,m2){
			var len=(runenv.eval(m2)|0)+1;
			var temp=Array(len);
			for(var i=0;i<len;i++){
				temp[i]=0;
			}
			eval(m1+"=temp;");
		};
		param.replace(/([A-Z])\[(.*?)\]/g,callback);
	};
	runenv.seekline=function(param){
		var i,line;
		try{
			line=eval(param);
			for(i=0;i<binary.length;i++){
				if (binary[i].command!="LINE") continue;
				if ((line|0)!=(binary[i].param|0)) continue;
				return i;
			}
			return -1;
		} catch(e) {
			for(i=0;i<binary.length;i++){
				if (binary[i].command!="LABEL") continue;
				if (param!=binary[i].param) continue;
				return i;
			}
			return -1;
		}
	};
	runenv.GOTO=function(param){
		var line;
		line=this.seekline(param);
		if (0<=line) {
			pc=line;
		} else {
			this.error("Line/Label ("+param+") not found");
		}
	};
	runenv.GOSUB=function(param){
		stack[sp++]=pc;
		this.GOTO(param);
	};
	runenv.RETURN=function(param){
		returnval=0+this.eval(param);
		pc=stack[--sp];
	};
	runenv.RESTORE=function(param){
		var line;
		line=this.seekline(param);
		if (0<=line) {
			dataline=line;
			datapos=-1;
		} else {
			this.error("Line/Label ("+param+") not found");
		}
	};
	runenv.FOR=function(){
		stack[sp++]=pc;
		pc=pc+1;
	};
	runenv.NEXT=function(param){
		if (this.eval(binary[stack[sp-1]-1].param)) sp--;
		else pc=stack[sp-1];
	};
	runenv.IF=function(param){
		ifexec=(this.eval(param)?1:0);
	};
	runenv.ELSE=function(){
		ifexec=(ifexec?0:1);
	};
	runenv.POKE=function(param){
		var m=basic.getvalue(param);
		var addr=this.eval(m[0]);
		var data=this.eval(m[1].substr(1));
		display.poke(addr,data);
	};
	runenv.DRAWCOUNT=function(param){
		interrupt.drawcount=this.eval(param);
	};
	runenv.CLS=function(){
		display.cls();
	};
	runenv.CURSOR=function(param){
		var m=basic.getvalue(param);
		var x=this.eval(m[0]);
		var y=this.eval(m[1].substr(1));
		display.cursor(x,y);
	};
	runenv.COLOR=function(param){
		display.color(this.eval(param));
	};
	runenv.BGCOLOR=function(param){
		var m=basic.getvalue(param);
		var r=this.eval(m[0]);
		m=basic.getvalue(m[1].substr(1));
		var g=this.eval(m[0]);
		var b=this.eval(m[1].substr(1));
		display.setbgcolor(r,g,b);
		refreshfont=1;
	};
	runenv.PALETTE=function(param){
		var m=basic.getvalue(param);
		var n=this.eval(m[0]);
		m=basic.getvalue(m[1].substr(1));
		var r=this.eval(m[0]);
		m=basic.getvalue(m[1].substr(1));
		var g=this.eval(m[0]);
		var b=this.eval(m[1].substr(1));
		display.setpalette(n,r,g,b);
	};
	runenv.MUSIC=function(param){
		var str=this.eval(param);
		music.set_music(str);
	};
	runenv.DATA=function(param){
		// Do nothing
	};
	runenv.SOUND=function(param){
		var i;
		var data=[];
		var dl=dataline;
		var dp=datapos;
		this.RESTORE(param);
		do {
			i=READ();
			data=data.concat([i]);
		} while (i&0xFFFF0000);
		music.set_sound(data);
		dataline=dl;
		datapos=dp;
	};
	var waituntil=-1;
	var waitstart=-1;
	sleep=0;
	runenv.WAIT=function(param){
		var period=this.eval(param);
		if (period==0) return;
		if (waituntil<0 || waitstart<0) {
			// First call of this function.
			waitstart=interrupt.drawcount;
			waituntil=waitstart+period;
			waituntil&=0xFFFF;
			pc--;
			sleep=1;
		} else if (waitstart<waituntil) {
			if (waitstart<=interrupt.drawcount && interrupt.drawcount<waituntil) {
				// Stil waiting
				pc--;
			} else {
				// Done. Let's go out.
				waitstart=waituntil=-1;
				sleep=0;
			}
		} else {
			if (waituntil<=interrupt.drawcount && interrupt.drawcount<waitstart) {
				// Done. Let's go out.
				waitstart=waituntil=-1;
				sleep=0;
			} else {
				// Stil waiting
				pc--;
			}
		}
	}
	runenv.SCROLL=function(param){
		var TVRAM=display.vram;
		var cursorcolor=display.currentpalette;
		var i;
		var m=basic.getvalue(param);
		var x=this.eval(m[0]);
		var y=this.eval(m[1].substr(1));
		var vector=y*WIDTH_X+x;
		if (vector<0) {
			// Copy data from upper address to lower address
			for(i=0-vector;i<WIDTH_X*WIDTH_Y;i++){
				TVRAM[i+vector]=TVRAM[i];
				TVRAM[WIDTH_X*WIDTH_Y+i+vector]=TVRAM[WIDTH_X*WIDTH_Y+i];
			}
		} else if (0<vector) {
			// Copy data from lower address to upper address
			for(i=WIDTH_X*WIDTH_Y-vector-1;0<=i;i--){
				TVRAM[i+vector]=TVRAM[i];
				TVRAM[WIDTH_X*WIDTH_Y+i+vector]=TVRAM[WIDTH_X*WIDTH_Y+i];
			}
		} else {
			return;
		}
		if (x<0) {
			// Fill blanc at right
			for(i=x;i<0;i++){
				for(j=WIDTH_X+i;j<WIDTH_X*WIDTH_Y;j+=WIDTH_X){
					TVRAM[j]=0x00;
					TVRAM[WIDTH_X*WIDTH_Y+j]=cursorcolor;
				}
			}
		} else if (0<x) {
			// Fill blanc at left
			for(i=0;i<x;i++){
				for(j=i;j<WIDTH_X*WIDTH_Y;j+=WIDTH_X){
					TVRAM[j]=0x00;
					TVRAM[WIDTH_X*WIDTH_Y+j]=cursorcolor;
				}
			}
		}
		if (y<0) {
			// Fill blanc at bottom
			for(i=WIDTH_X*(WIDTH_Y+y);i<WIDTH_X*WIDTH_Y;i++){
					TVRAM[i]=0x00;
					TVRAM[WIDTH_X*WIDTH_Y+i]=cursorcolor;
			}
		} else if (0<y) {
			// Fill blanc at top
			for(i=0;i<WIDTH_X*y;i++){
					TVRAM[i]=0x00;
					TVRAM[WIDTH_X*WIDTH_Y+i]=cursorcolor;
			}
		}
		display.refresh();
	}
	runenv.USEPCG=function(param){
		if (param=="") param="1";
		var mode=this.eval(param);
		display.usepcg(mode);
		refreshfont=1;
	}
	runenv.PCG=function(param){
		var m=basic.getvalue(param);
		var ascii=this.eval(m[0]);
		m=basic.getvalue(m[1].substr(1));
		var data1=this.eval(m[0]);
		var data2=this.eval(m[1].substr(1));
		display.pcg(ascii,data1,data2);
		refreshfont=1;
	}
	runenv.DEBUG=function(param){
		param=runenv.eval(param);
		alert(""+(typeof param)+"\n"+param);
	};
	// Functions
	var INT=function(param){
		if ((typeof param)!='number') return param;
		param=parseInt(param);
		if (isNaN(param)) throw("NaN detected");
		return param;
	};
	var PARSEINT=function(param){
		return INT(param);
	};
	var RNDSEED=0x3045;
	var RND=function(){
		var i,j,rnd;
		rnd=RNDSEED;
		for (i=0;i<15;i++) {
			j=rnd;
			j^=(j>>1);
			j^=(j>>2);
			j^=(j>>9);
			j>>=3;
			j&=0x01;
			j|=rnd<<1;
			j&=0x0000FFFF;
			rnd=j;
		}
		RNDSEED=rnd;
		return rnd&0x00007FFF;
	};
	var ABS=function(num){
		if (num<0) return 0-num;
		return num;
	};
	var SGN=function(num){
		if (0<num) return 1;
		if (num<0) return -1;
		return 0;
	};
	var NOT=function(num){
		return num?0:1;
	};
	var ASC=function(str){
		str=''+str;
		var b=str.charCodeAt(0);
		if (0xff00<b) b-=0xFF00-0x40; // Kana support
		return b;
	};
	var LEN=function(str){
		str=''+str;
		return str.length;
	};
	var STRNCMP=function(strX,strY,len){
		var x=(''+strX).substr(0,len);
		var y=(''+strY).substr(0,len);
		if (x<y) return -1;
		if (x==y) return 0;
		if (y<x) return 1;
	};
	var CHR=function(ascii){
		return String.fromCharCode(ascii);
	};
	var HEX=function(num,len){
		var str=num.toString(16);
		str=str.toUpperCase();
		if (arguments.length<2) return str;
		str="0000000"+str;
		return str.substr(0-len);
	};
	var DEC=function(num){
		return parseInt(num).toString(10);
	};
	var VAL=function(str){
		str=''+str;
		var ret=parseInt(str.replace(/^\$/, '0x'));
		if (isNaN(ret)) return 0;
		return ret;
	};
	var INPUTSTART=0;
	var INPUTVAL='';
	var INKEY=function(){
		return button.inkey;
	};
	var INPUT=function(){
		// TODO: shift key support.
		var char;
		if (!INPUTSTART) {
			// Wait until key will be released
			pc--;
			if (button.inkey==0) {
				// Set flag and return
				INPUTSTART=1;
				INPUTVAL='';
				button.got=0;
			}
			return '';
		}
		var str;
		switch(button.got){
			case 0:
				// Key is not pressed
				display.showcursor(1);
				pc--;
				return '';
			case 0x08:
				// BS
				button.got=0;
				pc--;
				if (INPUTVAL=='') return '';
				display.showcursor(0);
				display.printchar(0x20);
				display.cursorX--;
				display.cursorX--;
				display.printchar(0x20);
				display.cursorX--;
				display.showcursor(1);
				INPUTVAL=INPUTVAL.substr(0,INPUTVAL.length-1);
				return '';
			case 0x0d: case 0x0a:
				// Enter key
				display.showcursor(0);
				runenv.PRINT('""'); // New line.
				button.got=0;
				str=INPUTVAL+'';
				INPUTVAL='';
				INPUTSTART=0;
				return str;
			default:
				// Other keys
				char=String.fromCharCode(button.got);
				break;
		}
		display.showcursor(0);
		runenv.PRINT('"'+String.fromCharCode(button.got)+'";');
		INPUTVAL+=char;
		button.got=0;
		pc--;
		return '';
	};
	var SUBSTR=function(str,x,y){
		str=''+str;
		if (2<arguments.length) return str.substr(x,y);
		return str.substr(x);
	};
	var GOSUBFUNC=function(){
		return 0+returnval;
	};
	var TVRAM=function(addr){
		if (0<arguments.length) return display.peek(addr);
		return 0;
	};
	var PEEK=TVRAM;
	var DRAWCOUNT=function(){
		return interrupt.drawcount;
	};
	var KEYS=function(param){
		if (0<arguments.length) return button.bindata&param;
		return button.bindata&63;
	};
	var MUSIC=function(){
		return music.remaining();
	};
	var READ=function(){
		var ret,m;
		if (datapos<0) {
			do {
				dataline++;
			} while(binary[dataline].command!="DATA");
			datapos=0;
		}
		m=basic.getvalue(binary[dataline].param.substr(datapos));
		ret=eval(m[0]);
		if (m[1].charAt(0)==',') {
			datapos+=m[0].length+1;
		} else {
			datapos=-1;
		}
		return ret;
	};
	// eval function running here.
	runenv.eval=function(code){
		try{
			return eval(code);
		} catch(e) {
			alert(code);
			alert("eval() error\n"+e+"\n"+arguments.caller+"\n"+code);
		}
	};
	// Preparation of executing engine within this function.
	var exec=function(times){
		var i,command,param;
		for(i=0;i<times;i++){
			if (pc<binary.length) {
				command=binary[pc].command;
				param=binary[pc].param;
				pc++;
				if (ifexec || command=='LINE' || command=='LABEL'|| command=='ELSE') {
					if (runenv[command]) {
						runenv[command](param);
						continue;
					} else {
						runenv.error("Command not found: "+command);
					}
				}
			} else {
				break;
			}
			if (sleep) break;
		}
		// Some requests
		if (refreshfont) {
			refreshfont=0;
			display.refreshfont();
		}
		// Recall this function after 1 msec.
		if (pc<basic.binary.length) {
			setTimeout(function(){exec(times);},1);
		} else {
			button.stop();
		}
	};
	// Run the script.
	exec(400);
};// END "basic.run();"

