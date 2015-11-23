del *.o /s
del *.d /s
del *.dol
del *.elf
make --jobs=9
elf2dol swiss.elf swiss.dol
d0lz swiss.dol swiss-lz.dol -m
d0lz swiss.dol swiss-lz-viper.dol -m -v
mkdir dist
copy swiss-lz.dol dist
mv swiss.dol dist
mv swiss-lz-viper.dol dist
pause