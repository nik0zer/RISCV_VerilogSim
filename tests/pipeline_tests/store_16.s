.section .text
.global _start

_start:
    addi x1, x0, 1
    addi x5, x0, 7
    sd x1, 0x10(x0)
    sd x1, 0x18(x0)
    sb x5, 0x10(x0)
    ld x2, 0x10(x0)
    ld x2, 0x18(x0)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop