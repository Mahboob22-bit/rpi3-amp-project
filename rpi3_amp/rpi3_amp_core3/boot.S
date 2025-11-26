.section ".text.boot"

.global _start

_start:
    // Get CPU ID
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3        // Core ID in x0

    // AMP: Nur Core 3 läuft diesen Code!
    // Cores 0, 1, 2 werden von Linux genutzt
    cmp     x0, #3
    bne     core_halt           // Nicht Core 3? → halt

core3_start:
    // Stack für Core 3 setzen
    ldr     x1, =_stack_top
    mov     sp, x1
    
    // BSS löschen
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b
    
4:  // Jump zu C main
    bl      main
    
core_halt:
    wfe
    b       core_halt

.section ".bss"
.align 16
_stack_bottom:
    .space 4096
_stack_top:
