del *.o
mkdir built
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\base.S
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c sdgecko\sd.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\frag.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\usbgecko.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\dvdqueue.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\dvdinterface.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\adp.c
powerpc-eabi-gcc -O1 -ffunction-sections -mmultiple -mstring -c base\Stream.c
powerpc-eabi-gcc -O2 -ffunction-sections -mmultiple -mstring -c base\__DSPHandler.S
powerpc-eabi-gcc -O1 -ffunction-sections -mmultiple -mstring -c base\appldr_start.S
powerpc-eabi-gcc -O1 -ffunction-sections -mmultiple -mstring -c base\appldr.c
powerpc-eabi-ld -o sd.elf base.o frag.o sd.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o --section-start .text=0x80001000 --gc-sections
del *.o
powerpc-eabi-objdump -D sd.elf > built\sd_disasm.txt
powerpc-eabi-objcopy -O binary sd.elf sd.bin
bin2s sd.bin > sd_final.s
mv -f sd_final.s ../swiss/source/patches/SlotAB-SD.s
del *.bin
del *.elf
pause