del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c sdgecko\sd.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-gcc -O2 -c base\cardnull.c
powerpc-eabi-gcc -O2 -c base\usbgecko.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-ld -o sd.elf base.o frag.o sd.o cardnull.o dvdqueue.o usbgecko.o --section-start .text=0x80001800
del *.o
powerpc-eabi-objdump -D sd.elf > built\sd_disasm.txt
powerpc-eabi-objcopy -O binary sd.elf sd.bin
bin2s sd.bin > sd_final.s
mv sd_final.s built
del *.bin
del *.elf
pause