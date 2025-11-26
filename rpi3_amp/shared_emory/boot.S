/* boot.S */
.section ".text.boot"
.global _start

_start:
    /* Stack Pointer für Core 3 setzen (wir nehmen 0x20008000 als Stack Top) */
    mov     sp, #0x20000000
    add     sp, sp, #0x8000  /* Stack size 32KB */

    /* Springe zu main C code */
    bl      main

    /* Falls main zurückkehrt (sollte nicht passieren), loop */
hang:
    wfe
    b       hang
