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

main=function(maxspeed,breakpoint){
	maxspeed=parseInt(maxspeed);
	breakpoint=parseInt(breakpoint);
	var speed=1;
	var time=new Date().getTime();
	var lastint=time;
	setTimeout(function(){
			var i;
			var from=time;
			var to=time=new Date().getTime();
			var msec=to-from;
			if (msec<20 && speed<maxspeed) {
				speed<<=1;
				if (maxspeed<speed) speed=maxspeed;
			} else if (50<msec) {
				speed>>=1;
				if (speed<1) speed=1;
			}
			// Show the current speed
			if (msec<20) {
				dom.getElement('speed').innerHTML='clock: '+parseInt(speed*1000/5)+' hz';
			} else {
				dom.getElement('speed').innerHTML='clock: '+parseInt(speed*(1000/msec))+' hz';
			}
			// Execute MIPS32 1/4 times
			// This corresponds to using NTSC video in MachiKania
			var exectimes=speed>>2;
			for(i=0;i<exectimes;i++){
				mips32.exec();
				if (system.waitFlag || mips32.pc==breakpoint) break;
			}
			system.waitFlag=0;
			// Increment core timer for remaining time
			mips32.incTimer(speed*5/msec-i);
			// Things to do every 16 msec
			if (16<time-lastint) {
				// Refresh somethings every 15 msec
				display.all();
				// Interrupts
				// Always for T2 (vector 9) and CS0 (vector 1)
				SFR.IFS0SET((1<<9)+(1<<1));
				interrupt.check();
				// Decide next time
				if (100>time-lastint) lastint-=16;
				else lastint=time;
			}
			// Check halt state
			if (mips32.checkHalt()||system.exceptionFlag||mips32.pc==breakpoint) {
				mips32.logreg();
			} else {
				setTimeout(arguments.callee,5);
			}
		},5);
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
	main(95454.533*5,breakpoint);// 95.4545 MHz
};

if (get.debug) {
	dom.getElement("debug").style.display="block";
} else {
	main(95454.533*5,0);// 95.4545 MHz
}
