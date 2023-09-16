
OUTPUT_ARCH(arm)

MEMORY
{
	dma_mem       (rwx) : ORIGIN = 0x00000800, LENGTH =  6K
	rom   		  (rwx) : ORIGIN = 0x00100000, LENGTH =  16M
	ram 		  (rwx) : ORIGIN = 0x02000000, LENGTH =  (424)*1024
}

ENTRY(__init)

SECTIONS
{

	.init.text : 
	{ 
		*(.init)
	} >rom

	.init.data :
	{
		*(.init.data) 
	} >rom

	dma_data = .;
	.dma_ram :
	{
		__dma_data_start = .;
		*(.dma_ram)
		__dma_data_end = .;
	} >dma_mem AT>rom

	. = ALIGN(8);
	
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
	} >rom
	
	.ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >rom

	.ARM.exidx : {
	    __exidx_start = .;
	    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
	    __exidx_end = .;
	} >rom

. = ALIGN(8);

	__init_data = .;
	
	.data : 
	{
	__data_start__ = .;
	*(.data .data.*)
		*(.gnu.linkonce.d.*)

		/* Need to pre-align so that the symbols come after padding */
		. = ALIGN(8);

		PROVIDE( __global_pointer$ = . + 0x800 );
		PROVIDE( _gp = . + 0x8000);
		*(.sdata .sdata.* .sdata2.*)
		*(.gnu.linkonce.s.*)
	
	__data_end__ = .;
	} >ram AT>rom



  .bss :
  {
	. = ALIGN(32 / 8);
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
  } >ram AT>rom
   /*
	. = ALIGN(8);
	ucHeap = .;
	ucHeapEnd = ORIGIN(ram) + LENGTH(ram) - 4;
	ucHeapSz = ucHeapEnd - ucHeap;
*/
	
	. = ALIGN(8);
	heap_start = .;
	heap_end = ORIGIN(ram) + LENGTH(ram) - 4;

  .ARM.attributes 0 : { *(.ARM.attributes) }


}
