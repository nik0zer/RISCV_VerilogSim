.section .text
.global _start

_start:
    addi x1, x0, 1
    add x1, x1, x1
    add x1, x1, x1
    add x1, x1, x1
    slti x2, x1, 5
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
    nop
    nop
    nop
    nop
    sret
