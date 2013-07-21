del *.o
del swiss-lz.s
bin2s ..\swiss\swiss-lz.dol > swiss-lz.s
powerpc-eabi-gcc -O2 -c startup.s
powerpc-eabi-gcc -O2 -c swiss-lz.s
powerpc-eabi-gcc -O2 -c main.c
powerpc-eabi-ld -o sdloader.elf startup.o main.o swiss-lz.o --section-start .text=0x81700000
del *.o
powerpc-eabi-objcopy -O binary sdloader.elf SDLOADER.BIN
del *.elf
del swiss-lz.s
pause