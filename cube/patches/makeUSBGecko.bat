set PATH=C:\devkitPro\devkitPPC\bin;%PATH%
del *.o
mkdir built
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\base.S
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c usbgecko\usbgecko.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\dvdqueue.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\dvdinterface.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\adp.c
powerpc-eabi-gcc -O1 -ffunction-sections -mmultiple -mstring -c base\Stream.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\__DSPHandler.S
powerpc-eabi-gcc -O1 -ffunction-sections -mmultiple -mstring -c base\appldr_start.S
powerpc-eabi-gcc -O1 -ffunction-sections -mmultiple -mstring -c base\appldr.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\frag.c
powerpc-eabi-ld -o usbgecko.elf base.o usbgecko.o dvdqueue.o dvdinterface.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o frag.o --section-start .text=0x80001000 --gc-sections
del *.o
powerpc-eabi-objdump -d usbgecko.elf > built\usb_disasm.txt
powerpc-eabi-objcopy -O binary usbgecko.elf usbgecko.bin
bin2s usbgecko.bin > usbgecko_final.s
mv -f usbgecko_final.s ../swiss/source/patches/USBGeckoRead.s
del *.bin
del *.elf
pause