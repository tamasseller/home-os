
PROVIDE(FlashAddrLoc = 0x80020000);
PROVIDE(FlashCmdLoc  = 0x80010000);
PROVIDE(FlashDataLoc = 0x80000000);  

MEMORY
{
    sram (rwx) : ORIGIN = 0x20000000, LENGTH = 8k
    flash (rx) : ORIGIN = 0x08000000, LENGTH = 64k
}

SECTIONS
{
	/DISCARD/ : {
		*(.ARM.exidx)
		*(.ARM.exidx.*)
		*(.ARM.extab)
		*(.ARM.extab.*)
	}
	
	.text :
    {
        . = ALIGN(4);
        PROVIDE(stext = .);
        KEEP(*(.isr_vector))
        
	    PROVIDE_HIDDEN (preinit_array_start = .);
    	KEEP (*(.preinit_array))
    	PROVIDE_HIDDEN (preinit_array_end = .);
    	
    	PROVIDE_HIDDEN (init_array_start = .);
    	KEEP (*(SORT(.init_array.*)))
	    KEEP (*(.init_array ))
    	PROVIDE_HIDDEN (init_array_end = .);
    	
	   	PROVIDE_HIDDEN (fini_array_start = .);
    	KEEP (*(SORT(.fini_array.*)))
    	KEEP (*(.fini_array ))
	    PROVIDE_HIDDEN (fini_array_end = .);

        _text = .;
        
        *(.text .text.*)        
        *(.rodata .rodata.*)        
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gcc_except_table)
        *(.gnu.linkonce.r.*)
        
		etext = .;
		sidata = etext;
        PROVIDE(etext = .);
    } > flash
    
    .stack (NOLOAD) : {
    	PROVIDE( sstack = . );
        . += 512;
        PROVIDE( mstack = . );
		. += 512;
		PROVIDE( pstack = . );
		PROVIDE( estack = . );

        . = ALIGN(4);
    } > sram
    
    .pdata (NOLOAD) : {
        *(.pdata)
        . = ALIGN(4);
    } > sram
    
    
    .data : AT (etext)
    {
        sdata = .;
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        edata = .;
    } > sram

    .bss (NOLOAD) : {
		. = ALIGN(4);
        sbss = .;
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);        
        ebss = .;
        . = ALIGN(4);
		PROVIDE(heapSpaceStart = .);
    } > sram

	PROVIDE(heapSpaceEnd = ABSOLUTE(0x20020000));
    end = .;
}
