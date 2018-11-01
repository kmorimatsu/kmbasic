option explicit
dim fileName, loadAddress, startAddress, ihxFileName, mztFileName

fileName="KM-BASIC"
loadAddress=&h1200
startAddress=&h1200
ihxFileName="release.ihx"
mztFileName="release.mzt"

makeMzt fileName,loadAddress,startAddress,hexData(ihxFileName,loadAddress),mztFileName
msgbox mztFileName & " is created." & vbCRLF & fileLength(ihxFileName,loadAddress) & "bytes"

function hexData(fname,loadAddr)
	Dim fobj, line, addr, b1, b2, b3
	dim hdata
	hdata=string(fileLength(fname,loadAddr)*2,"0")
	set fobj=CreateObject("Scripting.FileSystemObject").OpenTextFile(fname,1)
	while not fobj.AtEndOfStream
		line=fobj.ReadLine()
		if left(line,1)=":" then
			b1=cByte("&H" & mid(line,2,2)) 'length
			b2=cByte("&H" & mid(line,4,2)) 'address MSB
			b3=cByte("&H" & mid(line,6,2)) 'address LSB
			addr=b2*256+b3-loadAddr
			if 0<b2 then
				if addr<0 then
					msgbox "Attempt to initialize variables in code?"
					wscript.quit()
				end if
				hdata=left(hdata,addr*2) & mid(line,10,b1*2) & mid(hdata,addr*2+b1*2+1)
			end if
		end if
	wend
	hexData=hdata
end function

function fileLength(fname,loadAddr)
	Dim fobj, line, flen, b1, b2, b3
	set fobj=CreateObject("Scripting.FileSystemObject").OpenTextFile(fname,1)
	flen=loadAddr
	while not fobj.AtEndOfStream
		line=fobj.ReadLine()
		if left(line,1)=":" then
			b1=cByte("&H" & mid(line,2,2)) 'length
			b2=cByte("&H" & mid(line,4,2)) 'address MSB
			b3=cByte("&H" & mid(line,6,2)) 'address LSB
			if flen<b2*256+b3+b1 then flen=b2*256+b3+b1
		end if
	wend
	fileLength=flen-loadAddr
end function

sub makeMzt(fname,load,start,hData,saveFile)
	dim stream, bstream, i, fsize

	fsize=len(hData)/2

	fname=left(fname,15)
	for i=1 to 15
		fname=fname & chr(&h0D)
	next
	fname=left(fname,16)

	set bstream=new ByteStream
	set stream=CreateObject("ADODB.Stream")

	stream.open
	stream.type=1

	stream.write bstream.getByte(&h01) '0x01: machine code
	for i=1 to 16
		stream.write bstream.getByte(asc(mid(fname,i,1)))
	next
	stream.write bstream.getByte(&h00)
	stream.write bstream.getByte(fsize mod 256) 'byte size LSB
	stream.write bstream.getByte(fsize \ 256)   'byte size MSB
	stream.write bstream.getByte(load mod 256)  'load address LSB
	stream.write bstream.getByte(load \ 256)    'load address MSB
	stream.write bstream.getByte(start mod 256) 'start address LSB
	stream.write bstream.getByte(start \ 256)   'start address MSB
	for i=1 to 104
		stream.write bstream.getByte(&h00) 'Comment
	next
	for i=1 to fsize
		stream.write bstream.getByte(cByte("&h"&mid(hData,i*2-1,2)))
	next

	stream.SaveToFile saveFile,2
	stream.close
end sub

' Following class was fetched from:
' http://sei.qee.jp/docs/program/hta/sample/bstream.html

Const ENCODE_UNICODE = "unicode"

' StreamTypeEnum
Const adTypeBinary = 1
Const adTypeText = 2

'*********************************************************************
' ByteStreamクラス
' version 1.1
'*********************************************************************
Class ByteStream
	Private innerArray(255)
	'=================================================================
	' クラスの初期化処理
	'=================================================================
	Private Sub Class_Initialize()

		Dim wkStream
		Set wkStream = WScript.CreateObject("ADODB.Stream")
		wkStream.Type = adTypeText
		wkStream.Charset = ENCODE_UNICODE
		wkStream.Open

		Dim i
		For i=0 To &hff
			wkStream.WriteText ChrW(i)
		Next
		wkStream.Position = 0
		wkStream.Type = adTypeBinary

		If ("fe" = LCase(Hex(AscB(wkStream.Read(1))))) Then
			wkStream.Position = 2
		End If

		For i=0 To &hff
			wkStream.Position = wkStream.Position + 1
			innerArray(i) = wkStream.Read(1)
		Next

		wkStream.Close
		Set wkStream = Nothing
	End Sub
	'=================================================================
	' 指定した数値のByte()を返す
	'=================================================================
	Public Function getByte(num)
		If (num < 0) Or (UBound(innerArray) < num) Then
			getByte = innerArray(0) '0x00を返す
		Else
			getByte = innerArray(num)
		End If
	End Function
End Class
