
OUTPUT_ARCH(arm)

MEMORY
{
	rom  (rwx) : ORIGIN = 0x00000000, LENGTH = 20K
	rom2 (rwx) : ORIGIN = 0x00006000, LENGTH = 512K - 64K - 0x6000
	ram  (rwx) : ORIGIN = 0xD0006000, LENGTH = 512K - 64K - 0x6000
}

ENTRY(__init)                                                              

SECTIONS
{

	.init.text : 
	{ 
		*(.init)
	} >rom AT>rom

	.init.data :
	{
		*(.init.data)
		*(.init.usbram)
		*(.init.bss)
	} >rom AT>rom

	__dma_data_start = .;

	.dma_mem : 
	{
		*(.dma_mem.chains)
	} >rom AT>rom

	__dma_data_end = .;

	.usb_ram :
	{
		*(.usb_ram)
	} >rom AT>rom

	

	.text : 
	{


		/* code */
		*(.text.unlikely .text.unlikely.*)
		*(.text.startup .text.startup.*)
		*(.text .text.* .opd .opd.*)
		*(.gnu.linkonce.t.*)
		KEEP (*(.fini .fini.*))
		__text_end = .;

		PROVIDE (__etext = __text_end);
		PROVIDE (_etext = __text_end);
		PROVIDE (etext = __text_end);

		/* read-only data */
		*(.rdata)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.r.*)

		*(.srodata.cst16)
		*(.srodata.cst8)
		*(.srodata.cst4)
		*(.srodata.cst2)
		*(.srodata .srodata.*)
		*(.data.rel.ro .data.rel.ro.*)

		/* Need to pre-align so that the symbols come after padding */
		. = ALIGN(8);

		/* lists of constructors and destructors */
		PROVIDE_HIDDEN ( __preinit_array_start = . );
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN ( __preinit_array_end = . );

		PROVIDE_HIDDEN ( __init_array_start = . );
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN ( __init_array_end = . );

		PROVIDE_HIDDEN ( __fini_array_start = . );
		KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP (*(.fini_array .dtors))
		PROVIDE_HIDDEN ( __fini_array_end = . );

	} >ram AT>rom2
	

	
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
	} >ram AT>rom2


	.rodata  : 
	{
		*(.rodata) 
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
  } >ram AT>rom2
  . = ALIGN(32 / 8);
  
	
	__HEAP_START = .;
	/DISCARD/ : {
		*(.note .note.*)
		*(.eh_frame .eh_frame.*)
		*(.ARM.extab* .gnu.linkonce.armextab.*)
		*(.ARM.exidx*)
	}
}
