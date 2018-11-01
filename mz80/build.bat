@if exist error.txt del error.txt
@if exist *.ihx del *.ihx
@if exist *.asm del *.asm

sdcc %1 -mz80 -c
@if not errorlevel 1 goto skip
@pause
@echo error: %1>>error.txt
@goto end

:skip
@if exist error.txt goto end
sdcc *.rel -mz80 --code-loc 0x1200 --data-loc 0x1000 --no-std-crt0 -Wlcrt\crt.o
@ren *.ihx release.ihx
@if exist *.lk del *.lk
@if exist *.lst del *.lst
@if exist *.map del *.map
@if exist *.noi del *.noi
@if exist *.rst del *.rst
@if exist *.sym del *.sym
@if exist *.cdb del *.cdb
@if exist *.mem del *.mem
@if exist *.omf del *.omf
ihx2mzf.vbs
:end