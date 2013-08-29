target remote 127.0.0.1:2331
monitor reset
load
set $pc = *0x20000004
set $sp = *0x20000000
c
bt