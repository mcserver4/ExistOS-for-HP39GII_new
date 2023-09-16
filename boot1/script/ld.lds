
pgt_size = (1+5)*4096;
page_frame_size = 424K;
exception_stack_sz	= 4K; 

OUTPUT_ARCH(arm)

MEMORY
{
	head_ram  	(rwx) : ORIGIN = 0xD0070000, LENGTH = 0x60
	rom2  		(rwx) : ORIGIN = 0xD0070060, LENGTH = 512K - 0x60
	ram 		(rwx) : ORIGIN = 0xD0002000, LENGTH = 512K - 0x2000
}

ENTRY(__init)

SECTIONS
{

	.init.text : 
	{ 
		*(.init)
	} >head_ram AT>head_ram

	.init.data :
	{
		*(.init.data) 
		*(.init.bss)
	} >head_ram AT>head_ram

	
	.text : 
	{ 
		/* code */
		*(.text .text.*)
		*(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
    	*(.eh_frame)
    	KEEP (*(.init))
    	KEEP (*(.fini))
		/* read-only data */
		*(.rdata)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.r.*)
		/* Need to pre-align so that the symbols come after padding */
		. = ALIGN(8);

	} >ram AT>rom2
	
	.ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >ram AT>rom2

	.ARM.exidx : {
	    __exidx_start = .;
	    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
	    __exidx_end = .;
	} >ram AT>rom2

	
	.data : 
	{
	__data_start__ = .;
	*(.data .data.*)
		*(.gnu.linkonce.d.*)

		/* Need to pre-align so that the symbols come after padding */
		. = ALIGN(8); 
		*(.sdata .sdata.* .sdata2.*)
		*(.gnu.linkonce.s.*)
	
	__data_end__ = .;
	} >ram AT>rom2
 
  .bss  ALIGN(4) :
  {
	__bss_start = .;
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
   *(COMMON)
   /* Align here to ensure that the .bss section occupies space up to
      _end.  Align after .bss to ensure correct alignment even if the
      .bss section disappears because there are no input sections.  */
   . = ALIGN(32 / 8);
   __bss_end = .;
  } >ram 

   /*
	. = ALIGN(8);
	__HEAP_START = .;
	. = . + heap_size;
	__HEAP_END = .;*/

	__HEAP_END = .;
	. = ALIGN(4);
	. = . + exception_stack_sz;
	__BOOT1_HEAD_END = .;

	
  	. = ALIGN(4096);
	pgt_start = .;
	. = . + pgt_size;
	pgt_end = .;
  	. = ALIGN(4096);
	page_frame_start = .;
	. = . + page_frame_size;
	page_frame_end = .;

 
}
