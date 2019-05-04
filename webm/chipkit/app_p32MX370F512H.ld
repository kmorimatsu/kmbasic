/*
	Linker script for MachiKania web
*/

OUTPUT_FORMAT("elf32-tradlittlemips")
OUTPUT_ARCH(pic32mx)
ENTRY(_reset)
PROVIDE(_min_heap_size = 0x800);
PROVIDE(_min_stack_size = 0x800) ;
INPUT("processor.o")
PROVIDE(_vector_spacing = 0x00000001);
_ebase_address = 0x9D006000;
_RESET_ADDR                    = (0x9D006000 + 0x1000);
_BEV_EXCPT_ADDR                = (0x9D006000 + 0x1000 + 0x380);
_DBG_EXCPT_ADDR                = (0x9D006000 + 0x1000 + 0x480);
_DBG_CODE_ADDR                 = 0xBFC02000;
_DBG_CODE_SIZE                 = 0xFF0;
_GEN_EXCPT_ADDR                = _ebase_address + 0x180;

MEMORY
{
  kseg0_program_mem     (rx)  : ORIGIN = (0x9D006000+0x1000+0x490), LENGTH = (0x80000-0x6000-0x1000-0x490-0x1000)
  kseg0_boot_mem              : ORIGIN = (0x9D006000+0x1000+0x490), LENGTH = 0 /* dummy */
  exception_mem               : ORIGIN = 0x9D006000, LENGTH = 0x1000
  kseg1_boot_mem              : ORIGIN = (0x9D006000+0x1000), LENGTH = 0x490
  debug_exec_mem              : ORIGIN = 0xBFC02000, LENGTH = 0xFF0
  devcfg               (r)   : ORIGIN = 0x9fc02ff0, LENGTH = 16
  kseg1_data_mem       (w!x)  : ORIGIN = 0xA0000000, LENGTH = 0x20000
  sfrs                        : ORIGIN = 0xBF800000, LENGTH = 0x100000
  configsfrs                  : ORIGIN = 0xBFC02FF0, LENGTH = 0x10
}


SECTIONS
{
  .config         :
  {
    *(.config3)
    *(.config2)
    *(.config1)
    *(.config0)
  } > devcfg
}
SECTIONS
{
  .reset _RESET_ADDR :
  {
    KEEP(*(.reset))
    KEEP(*(.reset.startup))
  } > kseg1_boot_mem
  .app_excpt _GEN_EXCPT_ADDR : { KEEP(*(.gen_handler)) } > exception_mem
  .vector_0  _ebase_address + 0x200 : { KEEP(*(.vector_0 )) } >exception_mem
  .vector_1  _ebase_address + 0x220 : { KEEP(*(.vector_1 )) } >exception_mem
  .vector_2  _ebase_address + 0x240 : { KEEP(*(.vector_2 )) } >exception_mem
  .vector_3  _ebase_address + 0x260 : { KEEP(*(.vector_3 )) } >exception_mem
  .vector_4  _ebase_address + 0x280 : { KEEP(*(.vector_4 )) } >exception_mem
  .vector_5  _ebase_address + 0x2a0 : { KEEP(*(.vector_5 )) } >exception_mem
  .vector_6  _ebase_address + 0x2c0 : { KEEP(*(.vector_6 )) } >exception_mem
  .vector_7  _ebase_address + 0x2e0 : { KEEP(*(.vector_7 )) } >exception_mem
  .vector_8  _ebase_address + 0x300 : { KEEP(*(.vector_8 )) } >exception_mem
  .vector_9  _ebase_address + 0x320 : { KEEP(*(.vector_9 )) } >exception_mem
  .vector_10 _ebase_address + 0x340 : { KEEP(*(.vector_10)) } >exception_mem
  .vector_11 _ebase_address + 0x360 : { KEEP(*(.vector_11)) } >exception_mem
  .vector_12 _ebase_address + 0x380 : { KEEP(*(.vector_12)) } >exception_mem
  .vector_13 _ebase_address + 0x3a0 : { KEEP(*(.vector_13)) } >exception_mem
  .vector_14 _ebase_address + 0x3c0 : { KEEP(*(.vector_14)) } >exception_mem
  .vector_15 _ebase_address + 0x3e0 : { KEEP(*(.vector_15)) } >exception_mem
  .vector_16 _ebase_address + 0x400 : { KEEP(*(.vector_16)) } >exception_mem
  .vector_17 _ebase_address + 0x420 : { KEEP(*(.vector_17)) } >exception_mem
  .vector_18 _ebase_address + 0x440 : { KEEP(*(.vector_18)) } >exception_mem
  .vector_19 _ebase_address + 0x460 : { KEEP(*(.vector_19)) } >exception_mem
  .vector_20 _ebase_address + 0x480 : { KEEP(*(.vector_20)) } >exception_mem
  .vector_21 _ebase_address + 0x4a0 : { KEEP(*(.vector_21)) } >exception_mem
  .vector_22 _ebase_address + 0x4c0 : { KEEP(*(.vector_22)) } >exception_mem
  .vector_23 _ebase_address + 0x4e0 : { KEEP(*(.vector_23)) } >exception_mem
  .vector_24 _ebase_address + 0x500 : { KEEP(*(.vector_24)) } >exception_mem
  .vector_25 _ebase_address + 0x520 : { KEEP(*(.vector_25)) } >exception_mem
  .vector_26 _ebase_address + 0x540 : { KEEP(*(.vector_26)) } >exception_mem
  .vector_27 _ebase_address + 0x560 : { KEEP(*(.vector_27)) } >exception_mem
  .vector_28 _ebase_address + 0x580 : { KEEP(*(.vector_28)) } >exception_mem
  .vector_29 _ebase_address + 0x5a0 : { KEEP(*(.vector_29)) } >exception_mem
  .vector_30 _ebase_address + 0x5c0 : { KEEP(*(.vector_30)) } >exception_mem
  .vector_31 _ebase_address + 0x5e0 : { KEEP(*(.vector_31)) } >exception_mem
  .vector_32 _ebase_address + 0x600 : { KEEP(*(.vector_32)) } >exception_mem
  .vector_33 _ebase_address + 0x620 : { KEEP(*(.vector_33)) } >exception_mem
  .vector_34 _ebase_address + 0x640 : { KEEP(*(.vector_34)) } >exception_mem
  .vector_35 _ebase_address + 0x660 : { KEEP(*(.vector_35)) } >exception_mem
  .vector_36 _ebase_address + 0x680 : { KEEP(*(.vector_36)) } >exception_mem
  .vector_37 _ebase_address + 0x6a0 : { KEEP(*(.vector_37)) } >exception_mem
  .vector_38 _ebase_address + 0x6c0 : { KEEP(*(.vector_38)) } >exception_mem
  .vector_39 _ebase_address + 0x6e0 : { KEEP(*(.vector_39)) } >exception_mem
  .vector_40 _ebase_address + 0x700 : { KEEP(*(.vector_40)) } >exception_mem
  .vector_41 _ebase_address + 0x720 : { KEEP(*(.vector_41)) } >exception_mem
  .vector_42 _ebase_address + 0x740 : { KEEP(*(.vector_42)) } >exception_mem
  .vector_43 _ebase_address + 0x760 : { KEEP(*(.vector_43)) } >exception_mem
  .vector_44 _ebase_address + 0x780 : { KEEP(*(.vector_44)) } >exception_mem
  .vector_45 _ebase_address + 0x7a0 : { KEEP(*(.vector_45)) } >exception_mem
  .vector_46 _ebase_address + 0x7c0 : { KEEP(*(.vector_46)) } >exception_mem
  .vector_47 _ebase_address + 0x7e0 : { KEEP(*(.vector_47)) } >exception_mem
  .vector_48 _ebase_address + 0x800 : { KEEP(*(.vector_48)) } >exception_mem
  .vector_49 _ebase_address + 0x820 : { KEEP(*(.vector_49)) } >exception_mem
  .vector_50 _ebase_address + 0x840 : { KEEP(*(.vector_50)) } >exception_mem
  .vector_51 _ebase_address + 0x860 : { KEEP(*(.vector_51)) } >exception_mem
  .vector_52 _ebase_address + 0x880 : { KEEP(*(.vector_52)) } >exception_mem
  .vector_53 _ebase_address + 0x8a0 : { KEEP(*(.vector_53)) } >exception_mem
  .vector_54 _ebase_address + 0x8c0 : { KEEP(*(.vector_54)) } >exception_mem
  .vector_55 _ebase_address + 0x8e0 : { KEEP(*(.vector_55)) } >exception_mem
  .vector_56 _ebase_address + 0x900 : { KEEP(*(.vector_56)) } >exception_mem
  .vector_57 _ebase_address + 0x920 : { KEEP(*(.vector_57)) } >exception_mem
  .vector_58 _ebase_address + 0x940 : { KEEP(*(.vector_58)) } >exception_mem
  .vector_59 _ebase_address + 0x960 : { KEEP(*(.vector_59)) } >exception_mem
  .vector_60 _ebase_address + 0x980 : { KEEP(*(.vector_60)) } >exception_mem
  .vector_61 _ebase_address + 0x9a0 : { KEEP(*(.vector_61)) } >exception_mem
  .vector_62 _ebase_address + 0x9c0 : { KEEP(*(.vector_62)) } >exception_mem
  .vector_63 _ebase_address + 0x9e0 : { KEEP(*(.vector_63)) } >exception_mem

  .text           :
  {
    *(.exception)
    *(.text .stub .text.* .gnu.linkonce.t.*)
    *(.gnu.warning)
    *(.glue_7t) *(.glue_7)
    __rodata_start = . ;
    *(.rodata .rodata.* .gnu.linkonce.r.* .rel.dyn)
    __abiflags = . ;
    *(.MIPS.abiflags)
    *(.dinit)
    . = ALIGN (4);
    _etext = .;
  } > kseg0_program_mem
  .persist   :
  {
    _persist_begin = .;
    *(.persist .persist.*)
    *(.pbss .pbss.*)
    . = ALIGN(4) ;
    _persist_end = .;
  } >kseg1_data_mem
  _gp = ALIGN(16) + 0x7ff0;
  .got ALIGN(4) :
  {
    *(.got.plt) *(.got)
    . = ALIGN(4) ;
  } >kseg1_data_mem
  _data_end = . ;
  _bss_begin = . ;
  .sbss ALIGN(4) :
  {
    _sbss_begin = . ;
    *(.dynsbss)
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
    _sbss_end = . ;
    . = ALIGN(4) ;
  } >kseg1_data_mem
  .bss     :
  {
    *(.dynbss)
    *(COMMON)
   . = ALIGN(. != 0 ? 4 : 1);
  } >kseg1_data_mem
  . = ALIGN(4) ;
  _end = . ;
  _bss_end = . ;
}
