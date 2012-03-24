del *.o
mkdir built
powerpc-eabi-gcc -O0 -c ide-exi\hddread.c
powerpc-eabi-as base\base.S -o base.o
powerpc-eabi-ld -o hdd.elf base.o hddread.o 
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