/************************************
* MachiKania web written by Katsumi *
*      This script is released      *
*        under the LGPL v2.1.       *
************************************/

// Initialize system
hexfile.load();
system.init();
display.init(system.pFontData);
display.all();
system.reset('MACHIKAM.HEX');

main=function(maxspeed){
	var speed=1;
	var time=new Date().getTime();
	setTimeout(function(){
			var i;
			var from=time;
			var to=time=new Date().getTime();
			var msec=to-from;
			if (msec<50 && speed<maxspeed) {
				speed<<=1;
				if (maxspeed<speed) speed=maxspeed;
			} else if (100<msec) {
				speed>>=1;
				if (speed<1) speed=1;
			}
			// Show the current speed
			if (msec<25) {
				dom.getElement('speed').innerHTML='clock: '+parseInt(speed*1000/15)+' hz';
			} else {
				dom.getElement('speed').innerHTML='clock: '+parseInt(speed*(1000/msec))+' hz';
			}
			// Refresh somethings every 15 msec
			system.waitFlag=0;
			display.all();
			// Execute MIPS32 1/4 times
			// This corresponds to using NTSC video in MachiKania
			for(i=0;i<(speed>>2);i++){
				mips32.exec();
				if (system.waitFlag) break;
			}
			// Increment core timer for remaining time
			mips32.incTimer(speed*15/msec-i);
			// Interrupts
			// Always for T2 (vector 9) and CS0 (vector 1)
			SFR.IFS0SET((1<<9)+(1<<1));
			interrupt.check();
			
			// Check halt state
			if (mips32.checkHalt()||system.exceptionFlag) {
				mips32.logreg();
			} else {
				setTimeout(arguments.callee,15);
			}
		},15);
};
steprun=function(codenum){
	var i;
	for(i=0;i<codenum;i++){
		mips32.exec();
		if (system.waitFlag) {
			dom.log('wait');
			break;
		}
	}
	display.all();
	var t='PC: 0x';
	t+=(0>mips32.pc ? mips32.pc+0x100000000 : mips32.pc).toString(16);
	t+=' (0x';
	t+=system.read32(mips32.pc).toString(16);
	t+=')';
	dom.log(t);
};
breakat=function(breakpoint){
	var i;
	if (breakpoint.substr(0,2)!='0x') alert('Invalid break point '+breakpoint);
	breakpoint=parseInt(breakpoint.substr(2), 16);
	breakpoint&=0x0fffffff;
	for(i=0;i<100000;i++){
		if ((mips32.pc&0x0fffffff)==breakpoint) break;
		mips32.exec();
		if (system.waitFlag) {
			dom.log('wait');
			break;
		}
	}
	display.all();
	if (100000<=i) dom.log('Stooped with 100000 execution');
	var t='PC: 0x';
	t+=(0>mips32.pc ? mips32.pc+0x100000000 : mips32.pc).toString(16);
	t+=' (0x';
	t+=system.read32(mips32.pc).toString(16);
	t+=')';
	dom.log(t);
};

main(1431818);// 95.4545 MHz (1431818 times in 15 msec)

