/*
 * Linker script — STM32F103C8T6
 *   Flash : 64 KB @ 0x08000000
 *   SRAM  : 20 KB @ 0x20000000
 *
 * Viet rieng cho du an nay. Khong dung ban ST ship kem
 * (Source/Templates/gcc/linker/STM32F103XB_FLASH.ld): ban do mang copyright
 * Ac6 voi dieu khoan "Distribution of this file (unmodified or modified) is
 * not permitted", nen khong the commit vao repo. Ban do cung khai 128 KB
 * flash (bien the xB), khong dung cho C8T6.
 *
 * Symbol xuat ra o day phai khop voi startup_stm32f103xb.s cua ST:
 *   _estack, _sidata, _sdata, _edata, _sbss, _ebss
 */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

/* Dinh stack: Cortex-M3 nap SP tu entry dau tien cua vector table. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/*
 * Heap = 0. Du an dung static allocation (configSUPPORT_DYNAMIC_ALLOCATION = 0),
 * khong co malloc va khong co heap_*.c trong build. Giu o 0 de neu co code nao
 * lo keo malloc vao thi link se bao loi thay vi am tham an bot RAM.
 *
 * _Min_Stack_Size la stack cua MSP. Sau khi scheduler chay, MSP chi con phuc vu
 * ISR (moi Task co stack rieng trong .bss). 1 KB la gia tri khoi dau; con so
 * chot lai phai lay tu RAM budget trong app_config.h.
 */
_Min_Heap_Size  = 0x000;
_Min_Stack_Size = 0x400;

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } >FLASH

  /* Anh cua .data nam trong flash, Reset_Handler copy sang RAM. */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT>FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* Bao loi link neu phan RAM con lai khong du cho heap + stack. */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
