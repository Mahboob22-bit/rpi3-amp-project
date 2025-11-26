SECTIONS
{
    /* AMP: Code läuft in reserviertem Speicher für Core 3 */
    . = 0x20000000;
    
    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }
    
    .rodata : {
        *(.rodata*)
    }
    
    .data : {
        *(.data*)
    }
    
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
    
    __bss_size = (__bss_end - __bss_start) >> 3;
    
    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
    }
}
