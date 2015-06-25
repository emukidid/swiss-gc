del *.o
mkdir built
powerpc-eabi-gcc -O2 -c base\base.S
powerpc-eabi-gcc -O2 -c wkf\wkf.c
powerpc-eabi-gcc -O2 -c base\frag.c
powerpc-eabi-gcc -O2 -DDEBUG -c base\usbgecko.c
powerpc-eabi-gcc -O2 -c base\dvdqueue.c
powerpc-eabi-gcc -O2 -DDEBUG -c base\dvdinterface.c
powerpc-eabi-gcc -O2 -c base\adp.c
powerpc-eabi-gcc -O1 -c base\Stream.c
powerpc-eabi-gcc -O2 -c base\__DSPHandler.S
powerpc-eabi-gcc -O1 -c base\appldr_start.S
powerpc-eabi-gcc -O1 -c base\appldr.c
powerpc-eabi-ld -o wkf.elf base.o frag.o wkf.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o --section-start .text=0x80001000
del *.o
powerpc-eabi-objdump -D wkf.elf > built\wkf_disasm.txt
powerpc-eabi-objcopy -O binary wkf.elf wkf.bin
bin2s wkf.bin > wkf.s
mv -f wkf.s ../swiss/source/patches/wkfPatch.s
del *.bin
del *.elf
pause