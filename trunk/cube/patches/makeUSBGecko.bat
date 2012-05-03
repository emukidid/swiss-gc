del *.o
mkdir built
powerpc-eabi-gcc -O0 -c usbgecko\usbgecko.c
powerpc-eabi-as base\base.S -o base.o
powerpc-eabi-ld -o usbgecko.elf base.o usbgecko.o 
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