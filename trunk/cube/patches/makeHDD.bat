powerpc-eabi-as hdd.S -o dvdcode.elf
doltool -d dvdcode.elf
doltool -b dvdcode.dol
doltool -c dvdcode.bin 0x80001800 0x80001800
bin2s dvdcode.bin > READpatch.s
del *.bin
del *.elf
pause