del *.o
mkdir built
powerpc-eabi-as sdgecko\sd.S -o sd.o
powerpc-eabi-as base\base.S -o base.o
powerpc-eabi-gcc -O2 -c base\card.c
powerpc-eabi-gcc -O2 -c base\usbgecko.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-ld -o sd.elf base.o sd.o card.o dvdqueue.o usbgecko.o --entry 0x80001800 --section-start .text=0x80001800
del *.o
doltool -d sd.elf
doltool -b sd.dol
doltool -c sd.bin 0x80001800 0x80001800
bin2s sd.bin > sd_final.s
mv sd_final.s built
mv sd.dol built
del *.bin
powerpc-eabi-objdump -D sd.elf > dump.txt
pause