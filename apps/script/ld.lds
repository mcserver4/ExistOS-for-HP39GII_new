
_ram_start = 0x02080000;

MEMORY
{
  RAM    (rwx) : ORIGIN = _ram_start, LENGTH = 380K
  ROM    (rx ) : ORIGIN = 0x08000000, LENGTH = 32M
}

OUTPUT_ARCH(arm)
ENTRY(_init)

SECTIONS
{
	.init.text :
	{ 
		*(.init)
	} >ROM

	. = ALIGN(4);
  	.ctors          :
  	{
		PROVIDE(__ctors_start__ = .);
		
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
/*
  		KEEP (*(SORT(.ctors.*)))
  		KEEP (*(.ctors))
*/
		PROVIDE(__ctors_end__ = .);	
		  
  	} >ROM

	. = ALIGN(4);

 	.fini_array    :
 	{
 	  PROVIDE (__fini_array_start = .);
 	  KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
 	  KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
 	  PROVIDE (__fini_array_end = .);
 	} >ROM


		. = ALIGN(4);
	.text :	
	{
		. = ALIGN(4);

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
	} >ROM
 

		. = ALIGN(4);
	.ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    }  >ROM

		. = ALIGN(4);
	.ARM.exidx : {
	    __exidx_start = .;
	    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
	    __exidx_end = .;
	}  >ROM


	__data_at_rom = .;
	.data : 
	{
		. = ALIGN(4);
		__data_start = .;
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		CONSTRUCTORS
		. = ALIGN(4);
		
        __data_end = .;
	} >RAM AT >ROM
	. = ALIGN(8);

    .bss : {
		. = ALIGN(8);
        _sbss = .;
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        _ebss = .;
    } > RAM

	. = ALIGN(8);

	__HEAP_START = .;
	

}
