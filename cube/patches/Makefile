DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DIR)/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

DEST    = ../swiss/source/patches

DISASM    = disassembly

all: dvdpatch hddpatch sdpatch usbpatch wkfpatch

dvdpatch: clean build-dvd
hddpatch: clean build-hdd
sdpatch: clean build-sd
usbpatch: clean build-usb
wkfpatch: clean build-wkf

clean:
	@rm -rf *.o
	@rm -rf $(DISASM)/*
	@-rmdir $(DISASM)
	@mkdir $(DISASM)

build-sd:
	@echo Building SD Patch ...
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/base.S
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c sdgecko/sd.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/frag.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/usbgecko.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/dvdqueue.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/dvdinterface.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/adp.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/Stream.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/__DSPHandler.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/appldr_start.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/appldr.c
	@$(LD) -o sd.elf base.o frag.o sd.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o --section-start .text=0x80001000 --gc-sections
	@rm -rf *.o
	@$(OBJDUMP) -D sd.elf > $(DISASM)/sd_disasm.txt
	@$(OBJCOPY) -O binary sd.elf sd.bin
	@$(BIN2S) sd.bin > sd_final.s
	@mv -f sd_final.s $(DEST)/SlotAB-SD.s
	@rm *.bin
	@rm *.elf

build-hdd:
	@echo Building HDD Patch ...
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/base.S
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c ide-exi/hddread.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/frag.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/usbgecko.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/dvdqueue.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/dvdinterface.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/adp.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/Stream.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/__DSPHandler.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/appldr_start.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/appldr.c
	@$(LD) -o hdd.elf base.o frag.o hddread.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o --section-start .text=0x80001000 -gc-sections 
	@rm -rf *.o
	@$(OBJDUMP) -D hdd.elf > $(DISASM)/hdd_disasm.txt
	@$(OBJCOPY) -O binary hdd.elf hdd.bin
	@$(BIN2S) hdd.bin > hdd_final.s
	@mv -f hdd_final.s $(DEST)/SlotAB-HDD.s
	@rm *.bin
	@rm *.elf

build-dvd:
	@echo Building DVD Patch ...
	@$(CC) -O2 -c dvd/base.S
	@$(CC) -O2 -c dvd/dvd.c
	@$(CC) -O2 -c sdgecko/sd.c
	@$(CC) -O2 -c base/frag.c
	@$(CC) -O2 -c base/usbgecko.c
	@$(CC) -O2 -c base/dvdqueue.c
	@$(LD) -o dvd.elf base.o frag.o sd.o dvd.o dvdqueue.o usbgecko.o --section-start .text=0x80001800
	@rm -rf *.o
	@$(OBJDUMP) -D dvd.elf > $(DISASM)/dvd_disasm.txt
	@$(OBJCOPY) -O binary dvd.elf dvd.bin
	@$(BIN2S) dvd.bin > dvd_final.s
	@mv -f dvd_final.s $(DEST)/DVDPatch.s
	@rm *.bin
	@rm *.elf

build-usb:
	@echo Building USBGecko Patch ...
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/base.S
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c usbgecko/usbgecko.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/dvdqueue.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/dvdinterface.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/adp.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/Stream.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/__DSPHandler.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/appldr_start.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c base/appldr.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c base/frag.c
	@$(LD) -o usbgecko.elf base.o usbgecko.o dvdqueue.o dvdinterface.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o frag.o --section-start .text=0x80001000 --gc-sections
	@rm -rf *.o
	@$(OBJDUMP) -D usbgecko.elf > $(DISASM)/usb_disasm.txt
	@$(OBJCOPY) -O binary usbgecko.elf usbgecko.bin
	@$(BIN2S) usbgecko.bin > usbgecko_final.s
	@mv -f usbgecko_final.s $(DEST)/USBGeckoRead.s
	@rm *.bin
	@rm *.elf

build-wkf:
	@echo Building WKF Patch ...
	@$(CC) -O2 -c wkf/base.S
	@$(CC) -O2 -c wkf/wkf.c
	@$(CC) -O2 -c base/usbgecko.c
	@$(LD) -o wkf.elf base.o wkf.o usbgecko.o --section-start .text=0x80001800
	@rm -rf *.o
	@$(OBJDUMP) -D wkf.elf > $(DISASM)/wkf_disasm.txt
	@$(OBJCOPY) -O binary wkf.elf wkf.bin
	@$(BIN2S) wkf.bin > wkf.s
	@mv -f wkf.s $(DEST)/wkfPatch.s
	@rm *.bin
	@rm *.elf
