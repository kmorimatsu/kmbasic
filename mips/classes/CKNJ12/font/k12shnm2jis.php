<?php

/*

	Binary font file generator for Shinonome 12x12 font.
	Place 'shnmk12.bdf' in the same directory and run this script.
	The font file is used for EUC-JP.
	On 2/23/2019, Shinonome font is not available from: http://openlab.ring.gr.jp/efont/shinonome/
	but archive is found from: https://web.archive.org/

*/

$tfile=file_get_contents('./shnmk12.bdf');
$ftable=array();
preg_replace_callback('/STARTCHAR[\s]+([0-9a-f]{4})[\s\S]*?(([0-9a-f]{4}[\s]+){12})/',function($m) use(&$ftable){
	/* JIS 0x3835: Œ³ */
	/* example:
		STARTCHAR 3835
		ENCODING 14389
		SWIDTH 960 0
		DWIDTH 12 0
		BBX 12 12 0 -2
		BITMAP
		0000
		1f80
		0000
		0000
		7fe0
		0900
		0900
		0900
		1100
		1120
		2120
		40e0
		ENDCHAR
	*/
	$ftable[hexdec($m[1])]=preg_replace('/([0-9a-f]{3})[0-9a-f][\s]+/','$1',$m[2]);
},$tfile);
//print_r($ftable);exit;

$result='';
for($code=0x2121;$code<=0x7426;$code++){
	if (isset($ftable[$code])) {
		for($i=0;$i<36;$i+=2){
			$b=substr($ftable[$code],$i,2);
			$result.=chr(hexdec($b));
		}
	} else {
		$result.="\x00\x00\x00\x00\x00\x00\x00\x00\x00";
		$result.="\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	}
}

file_put_contents('./SHNMK12.JIS',$result);
