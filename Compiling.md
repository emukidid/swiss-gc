# Compiling #

1. Check out the code from SVN.

IF and only IF you change the assembly code for the SD/IDE-EXI/USBGecko re-directed DVD Reads, then do the following, otherwise, skip straight ahead to step 3.

2.You need to compile 3 separate patch codes - 1 for SD cards, 1 for USB Gecko, and 1 for IDE-EXI. These can be built in cube\patches using the .bat files. The built binary data is converted to .S files which will need to be copied from cube\patches\built\ into cube\swiss\source\patches and renamed to SlotAB-SD.s, SlotAB-HDD.s and USBGeckoRead.s respectively.

3.) Swiss uses a custom libfat with fragmentation support. Make sure to build the latest in cube\libfat-frag\ and to use that library instead of the standard libfat lib (put it in your libogc libs dir)

4.) To build Swiss now with all the latest patch codes, we simply enter cube\swiss and execute:
```
make
elf2dol swiss.elf swiss.dol
d0lz swiss.dol swiss-lz.dol -m
```

(Where d0lz is softdev's Dollz compressor)

Note, if you get any references to missing 576Progressive video, use the latest libOGC as you're missing certain late additions regarding 576 Progressive video.