.syntax unified
.thumb
.arch armv7-m

.globl delay_loop
.func delay_loop
delay_loop:
    subs r0, #1
    bne  delay_loop
    bx   lr
.endfunc
