del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c ide-exi\hddread.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-gcc -O2 -c base\dvdnull.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-ld -o hdd.elf base.o dvdnull.o hddread.o dvdqueue.o frag.o --section-start .text=0x80001800
del *.o
powerpc-eabi-objdump -D hdd.elf > built\hdd_disasm.txt
powerpc-eabi-objcopy -O binary hdd.elf hdd.bin
bin2s hdd.bin > hdd_final.s
mv hdd_final.s built\SlotAB-HDD.s
del *.bin
del *.elf
pause