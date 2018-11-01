/**********************************
* KM-BASIC web written by Katsumi *
*     This script is released     *
*       under the LGPL v2.1.      *
**********************************/

interrupt=new Object();
interrupt.drawcount=0;
interrupt.init=function(){
	setInterval(function(){interrupt.timer();},16.67);
};
interrupt.audioFreq=0;
interrupt.timer=function(){
	this.drawcount++;
	this.drawcount&=0xFFFF;
	music.interrupt();
	if (this.audioFreq) {
		audio.set(this.audioFreq);
		audio.start();
	} else {
		audio.stop();
	}
};
interrupt.audio=function(pr3){
	// PR3=2047 <-> 437 Hz
	if (0<pr3 && pr3<0xffff) {
		this.audioFreq=2048*437/(pr3+1);
	} else {
		this.audioFreq=0;
	}
};
