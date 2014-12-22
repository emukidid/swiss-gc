del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c dvd\dvd.c
powerpc-eabi-gcc -O2 -c sdgecko\sd.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-gcc -O2 -c base\usbgecko.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-ld -o dvd.elf base.o frag.o sd.o dvd.o dvdqueue.o usbgecko.o --section-start .text=0x80001800
del *.o
powerpc-eabi-objdump -D dvd.elf > built\dvd_disasm.txt
powerpc-eabi-objcopy -O binary dvd.elf dvd.bin
bin2s dvd.bin > dvd_final.s
mv dvd_final.s built\dvdpatch.s
del *.bin
del *.elf
pause