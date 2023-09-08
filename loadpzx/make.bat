@echo off
del %1.bin
del %1.ihx
del %1

sdcc -mz80 --reserve-regs-iy --opt-code-size --max-allocs-per-node 10000 ^
--nostdlib --nostdinc --no-std-crt0 --code-loc 8192 --data-loc 12288 %1.c

makebin -p %1.ihx %1.bin
dd if=%1.bin of=%1 bs=1 skip=8192


