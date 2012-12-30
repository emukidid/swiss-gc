del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c usbgecko\usbgecko.c
powerpc-eabi-gcc -O2 -c base\cardnull.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-ld -o usbgecko.elf base.o usbgecko.o cardnull.o dvdqueue.o frag.o --section-start .text=0x80001800
del *.o
powerpc-eabi-objdump -D usbgecko.elf > built\usb_disasm.txt
powerpc-eabi-strip -R .comment usbgecko.elf
doltool -d usbgecko.elf
doltool -b usbgecko.dol
bin2s usbgecko.bin > usbgecko_final.s
mv usbgecko_final.s built
mv usbgecko.dol built
del *.bin
del *.elf
pause