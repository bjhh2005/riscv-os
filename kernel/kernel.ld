OUTPUT_ARCH( "riscv" )
ENTRY( _start )

SECTIONS
{
  /* 内核加载地址 */
  . = 0x80200000;

  /* * 1. 代码段 (.text)
   * 必须把 entry 代码放在最前面
   */
  .text : {
    *(.text.entry)   /* 对应 entry.S 中的 .section .text.entry */
    *(.text .text.*)
    
    /* * 关键修复：强制页对齐！
     * 确保 etext 位于页边界，这样代码段和数据段不会共享同一个物理页。
     * 避免 R|X 和 R|W 权限冲突。
     */
    . = ALIGN(4096); 
    etext = .;
    PROVIDE(etext = .);
  }

  /* 2. 只读数据段 (.rodata) */
  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* small rodata */
    *(.rodata .rodata.*)
  }

  /* 3. 数据段 (.data) */
  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* small data */
    *(.data .data.*)
  }

  /* 4. BSS 段 (未初始化数据) */
  .bss : {
    . = ALIGN(16);
    _bss_start = .;
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    _bss_end = .;
  }

  /* 结束标记，同样按页对齐，作为物理内存分配的起点 */
  . = ALIGN(4096);
  end = .;
  PROVIDE(end = .);
}