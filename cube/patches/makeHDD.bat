del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c ide-exi\hddread.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-gcc -O2 -c base\usbgecko.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-gcc -O2 -c base\dvdinterface.c
powerpc-eabi-gcc -O2 -c base\adp.c
powerpc-eabi-gcc -O1 -c base\Stream.c
powerpc-eabi-gcc -O2 -c base\__DSPHandler.S
powerpc-eabi-gcc -O1 -c base\appldr_start.S
powerpc-eabi-gcc -O1 -c base\appldr.c
powerpc-eabi-ld -o hdd.elf base.o frag.o hddread.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o --section-start .text=0x80001000
del *.o
powerpc-eabi-objdump -D hdd.elf > built\hdd_disasm.txt
powerpc-eabi-objcopy -O binary hdd.elf hdd.bin
bin2s hdd.bin > hdd_final.s
mv -f hdd_final.s ../swiss/source/patches/SlotAB-HDD.s
del *.bin
del *.elf
pause