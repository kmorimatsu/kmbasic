.globl _init
.globl _main
.globl _printBreak
.globl _checkBreak
.globl _allocateMemory
.globl __allocateMemory
.globl _freeMemory
.globl __freeMemory
.globl _strncmp

CALL _init           ;$1200
LD SP,#0x10EE        ;$1203
JP _main             ;$1206
JP _printBreak       ;$1209
JP _checkBreak       ;$120C
JP __allocateMemory  ;$120F
JP __freeMemory      ;$1212
JP _strncmp          ;$1215

_printBreak:
CALL #0x0015
_checkBreak:
CALL #0x001E
RET NZ
JP 0x1203

__allocateMemory:
; Allocate memory with BC bytes
; Return the address of allocated memory as DE
PUSH BC
CALL _allocateMemory
LD D,H
LD E,L
POP BC
RET

__freeMemory:
; Free the last allocated memory starting with HL
PUSH HL
CALL _freeMemory
POP HL
RET
