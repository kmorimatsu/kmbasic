@set chipkit=C:\Users\kmorimatsu\Documents\At Home\PIC\PIC32\chipkit\mpide-0150
if not "%1"=="" "%chipkit%\hardware\pic32\compiler\pic32-tools\bin\pic32-gcc.exe"  -mprocessor=32MX350F256H -x c -c %1 -o%1.o -MMD -MF%1.d  -g -O1
@if not errorlevel 0 @pause
"%chipkit%\hardware\pic32\compiler\pic32-tools\bin\pic32-gcc.exe" -mprocessor=32MX350F256H *.o interface\*.o "%chipkit%\hardware\pic32\compiler\pic32-tools\pic32mx\lib\libm.a" -o"release.elf" -Wl,-L"%chipkit%\hardware\pic32\compiler\pic32-tools\pic32mx\lib",--script="app_p32MX370F512H.ld",--defsym=__MPLAB_BUILD=1,-Map="release.map"
"%chipkit%\hardware\pic32\compiler\pic32-tools\bin\pic32-bin2hex.exe" "release.elf"
@pause
