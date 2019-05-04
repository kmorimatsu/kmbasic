@set chipkit=C:\Users\kmorimatsu\Documents\At Home\PIC\PIC32\chipkit\mpide-0150
"%chipkit%\hardware\pic32\compiler\pic32-tools\bin\pic32-gcc.exe"  -mprocessor=32MX350F256H -x c -c %1 -o%1.o -MMD -MF%1.d  -g -O1
@if not errorlevel 0 @pause
@pause