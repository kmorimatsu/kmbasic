dim handle,result,i,j
set handle = CreateObject("ADODB.Stream")
handle.open
handle.type = 1
handle.LoadFromFile("release.mzt")

result="static const unsigned char tapeheader[]={"&vbcrlf
for i=1 to 8
	for j=1 to 16
		result=result & "0x" & right("0" & hex(ascb(handle.read(1))),2) & ","
	next
	result=result&vbcrlf
next
result=left(result,len(result)-3)&vbcrlf&"};"&vbcrlf
result=result&"static const unsigned char tapebody[]={"&vbcrlf
do until handle.eos
	for i=1 to 16
		result=result & "0x" & right("0" & hex(ascb(handle.read(1))),2) & ","
		if handle.eos then exit do
	next
	result=result&vbcrlf
loop
result=left(result,len(result)-1)&vbcrlf&"};"&vbcrlf
CreateObject("Scripting.FileSystemObject").OpenTextFile("release.c.txt",2,true).Write(result)
msgbox "release.c.txt was created." 
