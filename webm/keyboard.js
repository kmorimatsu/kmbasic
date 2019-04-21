/****************************************
*  PIC32MX emulator written by Katsumi  *
*        This script is released        *
*          under the LGPL v2.1.         *
****************************************/

keyboard=new Object();
keyboard.shiftkeys=function(){
	return 0;
};
keyboard.testArray=new Array(
' '.charCodeAt(0),
'R'.charCodeAt(0),
'E'.charCodeAt(0),
'M'.charCodeAt(0),
' '.charCodeAt(0),
'T'.charCodeAt(0),
'E'.charCodeAt(0),
'S'.charCodeAt(0),
'T'.charCodeAt(0)
);
keyboard.testpos=0;
keyboard.ps2readkey=function(){
	var key;
	if (keyboard.testpos<keyboard.testArray.length) {
		key=keyboard.testArray[keyboard.testpos++];
	} else {
		key=0;
	}
	system.write16(system.pVkey,key);
	return key;
};