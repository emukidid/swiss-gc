del *.o
mkdir built
powerpc-eabi-gcc -O0 -c usbgecko\usbgecko.c
powerpc-eabi-as base\base.S -o base.o
powerpc-eabi-gcc -O0 -c base\cardnull.c
powerpc-eabi-ld -o usbgecko.elf base.o usbgecko.o cardnull.o --entry 0x80001800 --section-start .text=0x80001800
del *.o
doltool -d usbgecko.elf
doltool -b usbgecko.dol
doltool -c usbgecko.bin 0x80001800 0x80001800
bin2s usbgecko.bin > usbgecko_final.s
mv usbgecko_final.s built
mv usbgecko.dol built
del *.bin
del *.elf
pause