del *.o
mkdir built
powerpc-eabi-gcc -O0 -c ide-exi\hddread.c
powerpc-eabi-as base\base.S -o base.o
powerpc-eabi-gcc -O0 -c base\cardnull.c
powerpc-eabi-ld -o hdd.elf base.o hddread.o cardnull.o  --entry 0x80001800 --section-start .text=0x80001800
del *.o
doltool -d hdd.elf
doltool -b hdd.dol
doltool -c hdd.bin 0x80001800 0x80001800
bin2s hdd.bin > hdd_final.s
mv hdd_final.s built
mv hdd.dol built
del *.bin
del *.elf
pause