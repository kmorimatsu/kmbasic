/********************************
*    This script is released    *
*      under the LGPL v2.1.     *
********************************/

dom=Object();
dom.getElement=function(id){
	var ret=document.getElementById(id);
	return ret;
};
dom.getContext=function(id){
	var ret=document.getElementById(id).getContext("2d");
	return ret;
};
dom.x=function(obj,e){
	var x;
	if (typeof e.pageX != "undefined") { 
	  x = e.pageX;
	} else {
	  x = e.clientX + document.body.scrollLeft + document.documentElement.scrollLeft; 
	}
	x -= obj.offsetLeft;
	return x;
};
dom.y=function(obj,e){
	var y;
	if (typeof e.pageY != "undefined") { 
	  y = e.pageY;
	} else {
	  y = e.clientY + document.body.scrollTop + document.documentElement.scrollTop; 
	}
	y -= obj.offsetTop;
	return y;
};