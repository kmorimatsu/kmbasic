/********************************
*    This script is released    *
*      under the LGPL v2.1.     *
********************************/

main=function(){
	var text=dom.getElement('textarea').value;
	var prefix=dom.getElement('prefix').value;
	var pcg=0x80;
	var pcgscript='USEPCG\n';
	var printscript='';
	var zoeatext=zoeaprintscript='';
	var datascript='DATA ';
	var mainscript='';
	var line='';
	var i32='';
	var linenum=0;
	var jpnfnt=new Array();
	var i,j;
	var newline=function(){
		// New line
		if (0<i32.length) {
			if (datascript=='DATA ') {
				datascript+='0x'+i32;
			} else {
				datascript+=',0x'+i32;
			}
			i32='';
		}
		if (datascript!='DATA ') {
			linenum++;
			mainscript+='REM '+line+'\n';
			mainscript+='LABEL '+prefix+linenum.toString(10)+'\n';
			if (0<i32.length) {
				mainscript+=datascript+',0x'+i32+',0\n';
			} else {
				mainscript+=datascript+',0\n';
			}
			printscript+='REM '+line+'\n';
			printscript+='restore '+prefix+linenum.toString(10)+':gosub JPN:print\n';
			zoeaprintscript+='REM '+line+'\n';
			zoeaprintscript+='print "'+zoeatext+'"\n';
			zoeatext='';
		}
		line='';
		i32='';
		datascript='DATA ';
	};
	for (i=0;i<text.length;i++) {
		var c=text.charAt(i);
		var b=text.charCodeAt(i);
		if (b<0x20) {
			// New line
			newline();
			continue;
		}
		line+=c;
		if (0x80<=b) {
			var font=fontdata[b];
			if (!font) {
				alert('Font not found: '+c+'(0x'+b.toString(16)+')');
				return;
			}
			b=-1;
			for(var key in jpnfnt){
				key=parseInt(key);
				if (jpnfnt[key]==font) {
					b=key;
				}
			}
			if (b==-1) {
				if (pcg==0x20) {
					alert('Too many Japanese characters!');
					return;
				}
				pcgscript+='PCG 0x';
				if (pcg<0x10) pcgscript+='0';
				pcgscript+=pcg.toString(16);
				pcgscript+=','+font+'\n';
				b=pcg;
				jpnfnt[pcg]=font;
				pcg++;
				if (pcg==0x100) {
					pcg=0x01;
				} else if (pcg==0x0d || pcg==0x0a || pcg==0x87) {
					// Skip enter codes.
					// Skip 0x87. This is used for input prompt.
					pcg++;
				}
			}
		}
		if (b<16) {
			i32=''+'0'+b.toString(16)+i32;
			zoeatext+='\\x0'+b.toString(16);
		} else {
			i32=''+b.toString(16)+i32;
			zoeatext+='\\x'+b.toString(16);
		}
		if (5<i32.length) {
			if (datascript=='DATA ') {
				datascript+='0x'+i32;
			} else {
				datascript+=',0x'+i32;
			}
			i32='';
		}
	}
	newline();
	if (dom.getElement('toZoea').checked) {
		mainscript=pcgscript+'\n'+zoeaprintscript;
		mainscript+='A$=INPUT$()\n';
		mainscript+='end\n';
	} else {
		mainscript=pcgscript+'\n'+mainscript+'\n'+printscript;
		mainscript+='A$=INPUT$()\n';
		mainscript+='end\n';
		mainscript+='\n';
		mainscript+='label JPN\n';
		mainscript+='d=read()\n';
		mainscript+='if d=0 then return\n';
		mainscript+='label JPN2\n';
		mainscript+='  print chr$(d % 256);:d=d/256\n';
		mainscript+='if 0<d then JPN2\n';
		mainscript+='goto JPN\n';
	}
	dom.getElement('basiccode').value=mainscript;
};

clickzoea=function(obj){
	if (obj.checked) dom.getElement('labelprefix').hidden=true;
	else dom.getElement('labelprefix').hidden=false;
};
