/************************************
* MachiKania web written by Katsumi *
*      This script is released      *
*        under the LGPL v2.1.       *
************************************/

/*
	Public methods:
	display.init(FontData);
	display.write(addr,data);
*/

display=new Object();
display.fonts=new Image();
display.font=new Array(256);
display.init=function(FontData){
	var i,x,y,fd,h8,l8;
	// Set the contexts.
	this.context=dom.getContext("display");
	this.context.fillStyle   = "rgb(0, 0, 0)";
	this.context.fillRect(0,0,384,216);
	this.context.fillStyle   = "rgb(255, 255, 255)";
	// Show all fonts first
	for(i=0;i<256;i++){
		for(y=0;y<8;y++){
			fd=system.read8(FontData+i*8+y);
			for(x=0;x<8;x++){
				if (fd & (0x80>>x)) this.context.fillRect((i&15)*8+x,(i>>4)*8+y,1,1);
			}
		}
	}
	// Construction of images for font
	for (h8=0;h8<16;h8++) {
		for (l8=0;l8<16;l8++) {
			this.font[h8*16+l8]=this.context.getImageData(l8*8,h8*8,8,8);
		}
	}
};
display.all=function(){
	var data,posy,posx;
	this.readPos=this.writePos;
	for (posy=0;posy<27;posy++) {
		for (posx=0;posx<36;posx++) {
			if ((posx&3)==0) data=system.read32(system.pTVRAM+posy*36+posx);
			this.context.putImageData(this.font[(data>>((posx&3)*8))&255],posx<<3,posy<<3);
		}
	}
};
