/**********************************
* KM-BASIC web written by Katsumi *
*     This script is released     *
*       under the LGPL v2.1.      *
**********************************/

/*
	PR3=2047 <-> 437 Hz
*/

music=new Object();

/*
	c:  1722
	B:  1825
	A#: 1933
	A:  2048
	G#: 2170
	G:  2299
	F#: 2435
	F:  2580
	E:  2734
	D#: 2896
	D:  3067
	C#: 3251
	C:  3444
	Cb:	3650
*/
music.keys=Array();
music.keys[ 0]=[1933,1722,3251,2896,2580,2435,2170];//  0 7# C# A#m
music.keys[ 7]=[1933,1825,3251,2896,2580,2435,2170];//  7 6# F# D#m
music.keys[14]=[1933,1825,3251,2896,2734,2435,2170];// 14 5# B  G#m
music.keys[21]=[2048,1825,3251,2896,2734,2435,2170];// 21 4# E  C#m
music.keys[28]=[2048,1825,3251,3067,2734,2435,2170];// 28 3# A  F#m
music.keys[35]=[2048,1825,3251,3067,2734,2435,2299];// 35 2# D  Bm
music.keys[42]=[2048,1825,3444,3067,2734,2435,2299];// 42 1# G  Em
music.keys[49]=[2048,1825,3444,3067,2734,2580,2299];// 49 0  C  Am
music.keys[56]=[2048,1933,3444,3067,2734,2580,2299];// 56 1b F  Dm
music.keys[63]=[2048,1933,3444,3067,2896,2580,2299];// 63 2b Bb Gm
music.keys[70]=[2170,1933,3444,3067,2896,2580,2299];// 70 3b Eb Cm
music.keys[77]=[2170,1933,3444,3251,2896,2580,2299];// 77 4b Ab Fm
music.keys[84]=[2170,1933,3444,3251,2896,2580,2435];// 84 5b Db Bbm
music.keys[91]=[2170,1933,3650,3251,2896,2580,2435];// 91 6b Gb Ebm
music.keys[98]=[2170,1933,3650,3251,2896,2734,2435];// 98 7b Cb Abm
music.toneFlat=function(x){
	return x*1.0594630943592952645618252949463;
};
music.toneSharp=function(x){
	return x*0.94387431268169349664191315666753;
};
music.tones=music.keys[49];
music.qvalue=160;
music.lvalue=20;
music.mpoint=0;
music.mstr=0;
music.mspos=0;
music.music=Array(32);
music.sound=Array(32);
music.musiclen=Array(32);
music.soundlen=Array(32);
music.musicstart=0;
music.musicend=0;
music.musicwait=0;
music.soundstart=0;
music.soundend=0;
music.soundwait=0;
music.soundrepeat=0;
music.interrupt=function(){
	var i;
	// This function is called every 1/60 sec.
	if (this.soundstart!=this.soundend){
		// Start timer & OC4
		i=this.sound[this.soundstart];
		if (i<0xffff) {
			interrupt.audio(i);
		} else {
			interrupt.audio(0);
		}
		if ((--this.soundwait)<=0) {
			this.soundstart++;
			if (this.soundstart==this.soundend || 31<this.soundstart) {
				this.soundstart=0;
				this.soundrepeat--;
				if (0<this.soundrepeat) {
					this.soundwait=this.soundlen[this.soundstart];
				} else {
					this.soundend=this.soundrepeat=this.soundwait=0;
				}
			} else {
				this.soundwait=this.soundlen[this.soundstart];
			}
		}
		// Shift music data even though without output.
		if (this.musicstart!=this.musicend) {
			if ((--this.musicwait)<=0) {
				this.musicstart++;
				this.musicstart&=31;
				this.musicwait=this.musiclen[this.musicstart];
			}
		}
	} else if (this.musicstart!=this.musicend) {
		// Start timer & OC4
		i=this.music[this.musicstart];
		if (i<0xffff) {
			interrupt.audio(i);
		} else {
			interrupt.audio(0);
		}
		if ((--this.musicwait)<=0) {
			this.musicstart++;
			this.musicstart&=31;
			this.musicwait=this.musiclen[this.musicstart];
		}
	} else {
		// Stop timer
		interrupt.audio(0);
	}
}
music.remaining=function(){
	return (this.musicend-this.musicstart)&31;
};
music.GetNum=function(){
	var i, ret;
	var b;
	// Skip non number character
	for(i=0;(b=this.mstr.charAt(this.mspos+i))<'0' && '9'<this.mstr.charAt(this.mspos+i);i++);
	// Determine the number
	ret=0;
	while('0'<=b && b<='9'){
		ret*=10;
		ret+=b-'0';
		i++;
		b=this.mstr.charAt(this.mspos+i);
	}
	this.mspos+=i;
	return ret;
}

music.init=function(){
	// Initializations for music/sound.
	this.qvalue=160; // Q: 1/4=90
	this.lvalue=20;   // L: 1/8
	this.tones=this.keys[49]; // C major
	this.musicstart=this.musicend=this.musicwait=g_soundstart=g_soundend=g_soundwait=g_soundrepeat=0;
}

music.SetL=function(){
	// Set length of a character.
	// Syntax: L:n/m, where n and m are numbers.
	var n,m;
	n=this.GetNum();
	this.mspos++;
	m=this.GetNum();
	this.lvalue=this.qvalue*n/m;
}

music.SetQ=function(){
	var i;
	// Syntax: Q:1/4=n, where n is number.
	// Skip "1/4="
	for(i=0;this.mstr[this.mspos+i]!='=';i++);
	this.mspos+=i+1;
	i=this.GetNum();
	if      (i<48)  { this.qvalue=320; /* 1/4=45  */ }
	else if (i<53)  { this.qvalue=288; /* 1/4=50  */ }
	else if (i<60)  { this.qvalue=256; /* 1/4=56  */ }
	else if (i<70)  { this.qvalue=224; /* 1/4=64  */ }
	else if (i<83)  { this.qvalue=192; /* 1/4=75  */ }
	else if (i<102) { this.qvalue=160; /* 1/4=90  */ }
	else if (i<132) { this.qvalue=128; /* 1/4=113 */ }
	else if (i<188) { this.qvalue=96;  /* 1/4=150 */ }
	else            { this.qvalue=64;  /* 1/4=225 */ }
	this.lvalue=this.qvalue>>3;
}
music.SetK=function(){
	// Syntax: K:xxx
	switch(this.mstr.substr(this.mspos,3)){
		case "A#m":
			this.mspos+=3;
			music.tones=music.keys[0];
			return;
		case "D#m":
			this.mspos+=3;
			music.tones=music.keys[7];
			return;
		case "G#m":
			this.mspos+=3;
			music.tones=music.keys[14];
			return;
		case "C#m":
			this.mspos+=3;
			music.tones=music.keys[21];
			return;
		case "F#m":
			this.mspos+=3;
			music.tones=music.keys[28];
			return;
		case "Bbm":
			this.mspos+=3;
			music.tones=music.keys[84];
			return;
		case "Ebm":
			this.mspos+=3;
			music.tones=music.keys[91];
			return;
		case "Abm":
			this.mspos+=3;
			music.tones=music.keys[98];
			return;
		default:
			break;
	}
	switch(this.mstr.substr(this.mspos,2)){
		case "C#":
			this.mspos+=2;
			music.tones=music.keys[0];
			return;
		case "F#":
			this.mspos+=2;
			music.tones=music.keys[7];
			return;
		case "Bm":
			this.mspos+=2;
			music.tones=music.keys[35];
			return;
		case "Em":
			this.mspos+=2;
			music.tones=music.keys[42];
			return;
		case "Am":
			this.mspos+=2;
			music.tones=music.keys[49];
			return;
		case "Dm":
			this.mspos+=2;
			music.tones=music.keys[56];
			return;
		case "Gm":
		case "Bb":
			this.mspos+=2;
			music.tones=music.keys[63];
			return;
		case "Cm":
		case "Eb":
			this.mspos+=2;
			music.tones=music.keys[70];
			return;
		case "Fm":
		case "Ab":
			this.mspos+=2;
			music.tones=music.keys[77];
			return;
		case "Db":
			this.mspos+=2;
			music.tones=music.keys[84];
			return;
		case "Gb":
			this.mspos+=2;
			music.tones=music.keys[91];
			return;
		case "Cb":
			this.mspos+=2;
			music.tones=music.keys[98];
			return;
		default:
			break;
	}
	switch(this.mstr.charAt(this.mspos)){
		case "B":
			this.mspos++;
			music.tones=music.keys[14];
			return;
		case "E":
			this.mspos++;
			music.tones=music.keys[21];
			return;
		case "A":
			this.mspos++;
			music.tones=music.keys[28];
			return;
		case "D":
			this.mspos++;
			music.tones=music.keys[35];
			return;
		case "G":
			this.mspos++;
			music.tones=music.keys[42];
			return;
		case "C":
			this.mspos++;
			music.tones=music.keys[49];
			return;
		case "F":
			this.mspos++;
			music.tones=music.keys[56];
			return;
		default:
			break;
	}
}
music.SetM=function(){
	// Currently do nothing
	this.GetNum();
	this.GetNum();
}
music.set_sound=function(data){
	var sound;
	var len;
	var pos;
	// Initialize
	this.soundrepeat=this.soundstart=this.soundend=0;
	pos=0;
	while(data!=0){ // Exit if data is null.
		len=data[pos]>>16;
		sound=data[pos]&0x0000FFFF;
		if (len) {
			this.sound[pos]=sound-1;
			this.soundlen[pos]=len;
			pos++;
			if (32<pos) {
				alert("Sound data too long.");
				return;
			}
		} else {
			this.soundrepeat=sound;
			break;
		}
	}
	this.soundend=pos;
	this.soundwait=this.soundlen[0];
}

music.set_music=function(str){// Before modification for syntax: _ver024.zip
	var b;
	var tone,tonenatural;
	var len;
	this.mstr=str;
	this.mspos=0;
	while(this.mspos<this.mstr.length){
		b=this.mstr.charAt(this.mspos);
		if (this.mstr.charAt(this.mspos+1)==':') {
			// Set property
			this.mspos+=2;
			switch(b){
				case 'L':
					this.SetL();
					break;
				case 'Q':
					this.SetQ();
					break;
				case 'K':
					this.SetK();
					break;
				case 'M':
					this.SetM();
					break;
				default:
					alert(str);
					break;
			}	
		} else if ('A'<=b && b<='G' || 'a'<=b && b<='g' || b=='z') {
			this.mspos++;
			if (b=='z') {
				tone=0;
			} else if (b<='G') {
				tone=this.tones[b.charCodeAt(0)-'A'.charCodeAt(0)];
				tonenatural=this.keys[49][b.charCodeAt(0)-'A'.charCodeAt(0)];
			} else {
				tone=this.tones[b.charCodeAt(0)-'a'.charCodeAt(0)]>>1;
				tonenatural=this.keys[49][b.charCodeAt(0)-'a'.charCodeAt(0)]>>1;
			}
			// Check "'"s
			while(this.mstr.charAt(this.mspos)=='\''){
				this.mspos++;
				tone>>=1;
				tonenatural>>=1;
			}
			// Check ","s
			while(this.mstr.charAt(this.mspos)==','){
				this.mspos++;
				tone<<=1;
				tonenatural<<=1;
			}
			// Check "^","=","_"
			switch(this.mstr.charAt(this.mspos)){
				case '^':
					this.mspos++;
					tone=this.toneSharp(tone);
					break;
				case '_':
					this.mspos++;
					tone=this.toneFlat(tone);
					break;
				case '=':
					this.mspos++;
					tone=tonenatural;
					break;
				default:
					break;
			}
			// Check number for length
			b=this.mstr[this.mspos];
			if ('0'<=b && b<='9') {
				len=this.lvalue*this.GetNum();
			} else {
				len=this.lvalue;
			}
			if (this.mstr[this.mspos]=='/') {
				this.mspos++;
				len=len/this.GetNum();
			}
			// Update music value array
			if (this.musicstart==this.musicend) {
				this.musicwait=len;
			}
			this.music[this.musicend]=(tone-1)&0x0000FFFF;
			this.musiclen[this.musicend]=len;
			this.musicend++;
			this.musicend&=31;
		} else {
			alert('Music error :'+b+'/'+str);
		}
		// Go to next character
		while(0<this.mstr[this.mspos] && this.mstr[this.mspos]<=0x20 || this.mstr[this.mspos]=='|') this.mspos++;
	}
}