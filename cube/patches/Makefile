DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

DEST    = ../swiss/source/patches

DISASM    = disassembly

all: dvdpatch hddpatch sdpatch usbpatch wkfpatch bbapatch

dvdpatch: clean build-dvd
hddpatch: clean build-ideexi-v1 clean build-ideexi-v2
sdpatch: clean build-sd
usbpatch: clean build-usb
wkfpatch: clean build-wkf
bbapatch: clean build-bba

clean:
	@rm -rf *.o
	@rm -rf $(DISASM)/*
	@-rmdir $(DISASM)
	@mkdir $(DISASM)

build-sd:
	@echo Building SD Patch ...
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/base.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c sdgecko/sd.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/frag.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/igr.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/usbgecko.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/dvdqueue.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/dvdinterface.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/adp.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/Stream.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/__DSPHandler.S
	@$(LD) -o sd.elf base.o igr.o frag.o sd.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o --section-start .text=0x80001000 --gc-sections
	@rm -rf *.o
	@$(OBJDUMP) -D sd.elf > $(DISASM)/sd_disasm.txt
	@$(OBJCOPY) -O binary sd.elf sd.bin
	@$(BIN2S) sd.bin > sd_final.s
	@mv -f sd_final.s $(DEST)/SlotAB-SD.s
	@rm *.bin
	@rm *.elf

build-ideexi-v1:
	@echo Building IDE-EXI-v1 Patch ...
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/base.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c ide-exi/hddread.c -DIDE_EXI_V1
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/frag.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/igr.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/usbgecko.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/dvdqueue.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/dvdinterface.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/adp.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/Stream.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/__DSPHandler.S
	@$(LD) -o ideexi_v1.elf base.o igr.o frag.o hddread.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o --section-start .text=0x80001000 -gc-sections 
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_v1.elf > $(DISASM)/ideexi_v1_disasm.txt
	@$(OBJCOPY) -O binary ideexi_v1.elf ideexi_v1.bin
	@$(BIN2S) ideexi_v1.bin > ideexi_v1_final.s
	@mv -f ideexi_v1_final.s $(DEST)/SlotAB-IDEEXI_V1.s
	@rm *.bin
	@rm *.elf
	
build-ideexi-v2:
	@echo Building IDE-EXI-v2 Patch ...
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/base.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c ide-exi/hddread.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/frag.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/igr.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/usbgecko.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/dvdqueue.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/dvdinterface.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/adp.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/Stream.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/__DSPHandler.S
	@$(LD) -o ideexi_v2.elf base.o igr.o frag.o hddread.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o --section-start .text=0x80001000 -gc-sections 
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_v2.elf > $(DISASM)/ideexi_v2_disasm.txt
	@$(OBJCOPY) -O binary ideexi_v2.elf ideexi_v2.bin
	@$(BIN2S) ideexi_v2.bin > ideexi_v2_final.s
	@mv -f ideexi_v2_final.s $(DEST)/SlotAB-IDEEXI_V2.s
	@rm *.bin
	@rm *.elf

build-dvd:
	@echo Building DVD Patch ...
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c dvd/base.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c dvd/dvd.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c sdgecko/sd.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/frag.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/usbgecko.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/igr.c
	@$(LD) -o dvd.elf base.o igr.o frag.o sd.o dvd.o usbgecko.o --section-start .text=0x80001800
	@rm -rf *.o
	@$(OBJDUMP) -D dvd.elf > $(DISASM)/dvd_disasm.txt
	@$(OBJCOPY) -O binary dvd.elf dvd.bin
	@$(BIN2S) dvd.bin > dvd_final.s
	@mv -f dvd_final.s $(DEST)/DVDPatch.s
	@rm *.bin
	@rm *.elf

build-usb:
	@echo Building USBGecko Patch ...
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c usbgecko/base.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c usbgecko/usbgecko.c
	@$(LD) -o usbgecko.elf base.o usbgecko.o --section-start .text=0x80001000 --gc-sections
	@rm -rf *.o
	@$(OBJDUMP) -D usbgecko.elf > $(DISASM)/usb_disasm.txt
	@$(OBJCOPY) -O binary usbgecko.elf usbgecko.bin
	@$(BIN2S) usbgecko.bin > usbgecko_final.s
	@mv -f usbgecko_final.s $(DEST)/USBGeckoRead.s
	@rm *.bin
	@rm *.elf

build-wkf:
	@echo Building WKF Patch ...
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c wkf/base.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c wkf/wkf.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/usbgecko.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c sdgecko/sd.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/frag.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/igr.c  -DWKF
	
	@$(LD) -o wkf.elf base.o wkf.o usbgecko.o sd.o frag.o igr.o --section-start .text=0x80001000 --gc-sections
	@rm -rf *.o
	@$(OBJDUMP) -D wkf.elf > $(DISASM)/wkf_disasm.txt
	@$(OBJCOPY) -O binary wkf.elf wkf.bin
	@$(BIN2S) wkf.bin > wkf.s
	@mv -f wkf.s $(DEST)/wkfPatch.s
	@rm *.bin
	@rm *.elf

build-bba:
	@echo Building BBA Patch ...
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c usbgecko/base.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c bba/bba.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c bba/tcpip.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -fno-asynchronous-unwind-tables -c base/usbgecko.c
	@$(LD) -o bba.elf base.o bba.o tcpip.o usbgecko.o --section-start .text=0x80001000 --gc-sections
	@rm -rf *.o
	@$(OBJDUMP) -D bba.elf > $(DISASM)/bba_disasm.txt
	@$(OBJCOPY) -O binary bba.elf bba.bin
	@$(BIN2S) bba.bin > bba.s
	@mv -f bba.s $(DEST)/BBAPatch.s
	@rm *.bin
	@rm *.elf
	