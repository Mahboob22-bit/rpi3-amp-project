/* link.ld */
SECTIONS
{
    . = 0x20000000;  /* Startadresse im reservierten RAM */

    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
    }
}
