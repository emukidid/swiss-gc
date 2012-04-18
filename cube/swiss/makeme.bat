del *.o /s
del *.d /s
del *.dol
del *.elf
make
elf2dol swiss.elf swiss.dol
d0lz swiss.dol swiss-lz.dol -m
d0lz swiss.dol swiss-lz-viper.dol -m -v
mkdir dist
copy swiss.dol dist
copy swiss-lz.dol dist
copy swiss-lz-viper.dol dist
pause