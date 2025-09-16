OUTPUT_ARCH("riscv")
ENTRY(_entry)

SECTIONS {
    . = 0x80200000;
    
    .text : {
        *(.text .text.*)
    }
    
    .rodata : {
        *(.rodata .rodata.*)
    }
    
    .data : {
        *(.data .data.*)
    }
    
    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        __bss_end = .;
    }
    
    .stack : {
        . = ALIGN(4096);
        _stack_start = .;
        . += 16K;        /* 16KB 栈空间 */
        _stack_end = .;
    }
    
    _end = .;
}