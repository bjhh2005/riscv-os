OUTPUT_ARCH( "riscv" )
ENTRY( _start )

SECTIONS
{
  /* 内核加载地址 0x80000000 (必须与 entry.S 和 makefile 配合) */
  . = 0x80000000;

  .text : {
    *(.text.entry)
    *(.text .text.*)
    
    /* * [关键修复]
     * 必须将 trampoline.S 中的 .trampsec 段包含进来
     * 并且必须对齐到页边界 (4096)
     */
    . = ALIGN(4096);
    *(.trampsec)
    . = ALIGN(4096);

    etext = .;
    PROVIDE(etext = .);
  }

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*)
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    _bss_start = .;
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    _bss_end = .;
  }

  . = ALIGN(4096);
  end = .;
  PROVIDE(end = .);
}