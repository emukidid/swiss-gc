del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c sdgecko\sd.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-gcc -O2 -c base\card.c
powerpc-eabi-gcc -O2 -c base\usbgecko.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-ld -o sd.elf base.o frag.o sd.o card.o dvdqueue.o usbgecko.o --section-start .text=0x80001800
del *.o
powerpc-eabi-objdump -D sd.elf > built\sd_disasm.txt
powerpc-eabi-strip -R .comment sd.elf
doltool -d sd.elf
doltool -b sd.dol
bin2s sd.bin > sd_final.s
mv sd_final.s built
mv sd.dol built
del *.bin
del *.elf
pause