;/*****************************************************************************
; * @file     NVTLoader.ld
; * @brief    N9H31 link file
; *
; * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
; *****************************************************************************/

ENTRY(__vector)
/*
	We allocate stack size 0x9000 in Startup_GCC.S
*/
/* define stack size and heap size here */
stack_size = 0x9000;

/* define beginning and ending of stack */
_stack_start = LENGTH(RAM);
_stack_end = _stack_start - stack_size;
MEMORY
{
    RAM  (rwx)   : ORIGIN = 0x800000, LENGTH = 0x2000000
}

SECTIONS
{
    .text :
    {
        PROVIDE(__image_start = .);
        PROVIDE(__text_start = .);

        PROVIDE(__vectors_start = .);
        *(.vectors);
        . = ALIGN(4);
        PROVIDE(__vectors_end = .);
        *(.init);
        . = ALIGN(4);
        *(.text);
        . = ALIGN(4);
        *(.rodata);
        . = ALIGN(4);
        *(.rodata*);
        . = ALIGN(4);

        etext = .;

        PROVIDE(__text_end = .);
    } > RAM

    . = ALIGN(4);
    _etext = . ;
    PROVIDE (etext = .);

    .data : AT (_etext)
    {
        PROVIDE(__data_start__ = .);
        _data = . ;
        *(.data)
        . = ALIGN(4);
        PROVIDE(__data_end__ = .);
    } > RAM

    . = ALIGN(4);
    _edata = . ;
    PROVIDE (edata = .);

    sbss = .;
    .bss :
    {
        PROVIDE (__bss_start__ = .);
        *(.bss)
        *(.bss.**)
        *(COMMON)
        . = ALIGN(4);
        PROVIDE (__bss_end__ = .);
    }>RAM
    bss_size = __bss_end__ - __bss_start__;
    heap_size = _stack_start - __bss_end__;
    ebss = .;
    bss_size = ebss - sbss;

    . = ALIGN(8);	
    .heap :
    {
        PROVIDE (__heap_start__ = .);
        . = . + heap_size;
        end = .;
        PROVIDE (__heap_end__ = .);
    } >RAM
    
    __end__ =.;
}

