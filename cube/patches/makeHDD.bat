del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c ide-exi\hddread.c
powerpc-eabi-gcc -O2 -c base\cardnull.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-ld -o hdd.elf base.o hddread.o cardnull.o dvdqueue.o frag.o --section-start .text=0x80001800
del *.o
powerpc-eabi-objdump -D hdd.elf > built\hdd_disasm.txt
powerpc-eabi-strip -R .comment hdd.elf
doltool -d hdd.elf
doltool -b hdd.dol
bin2s hdd.bin > hdd_final.s
mv hdd_final.s built
mv hdd.dol built
del *.bin
del *.elf
pause