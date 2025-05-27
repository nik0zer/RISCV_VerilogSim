.section .text
.global _start

_start:
    addi x1, x0, 1
    addi x2, x0, 5
    nop
    nop
    nop
    sub x3, x2, x1
    add x4, x2, x1
    nop
    nop
    nop
    slti x5, x4, 10
    nop
    nop
    nop
    nop
    nop
    nop