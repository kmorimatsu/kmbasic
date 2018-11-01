@if exist *.asm del *.asm
sdcc %1 -mz80 -c
@if errorlevel 1 @pause
@for %%i in (*.asm) do %%i
@if exist *.lk del *.lk
@if exist *.lst del *.lst
@if exist *.map del *.map
@if exist *.noi del *.noi
:@if exist *.rel del *.rel
@if exist *.rst del *.rst
@if exist *.sym del *.sym
@if exist *.cdb del *.cdb
@if exist *.mem del *.mem
@if exist *.omf del *.omf
