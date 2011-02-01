del *.o /s
del *.d /s
del *.dol
del *.elf
make
elf2dol swiss.elf swiss.dol
d0lz swiss.dol swiss-lz.dol -m
pause