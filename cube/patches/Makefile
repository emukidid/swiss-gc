DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

OPTS	= -flto -ffunction-sections -fdata-sections -Wl,--gc-sections

DEST    = ../swiss/source/patches

DISASM    = disassembly

.NOTPARALLEL:
all: execdpatch dvdpatch hddpatch sdpatch usbpatch wkfpatch bbapatch

execdpatch: clean build-execD
dvdpatch: clean build-dvd
hddpatch: clean build-ideexi-v1 clean build-ideexi-altv1 clean build-ideexi-v2 clean build-ideexi-altv2
sdpatch: clean build-sd clean build-sd-alt
usbpatch: clean build-usb
wkfpatch: clean build-wkf
bbapatch: clean build-bba

clean:
	@rm -rf *.o
	@rm -rf $(DISASM)/*
	@-rmdir $(DISASM)
	@mkdir $(DISASM)

build-execD:
	@echo Building execD replacement ...
	@$(CC) -O1 $(OPTS) -c execd/appldr_start.S
	@$(CC) -O1 $(OPTS) -c execd/appldr.c
	@$(CC) -O1 $(OPTS) -o execdpatch.elf -T execd/appldr.ld -T base/common.ld appldr_start.o appldr.o
	@rm -rf *.o
	@$(OBJDUMP) -D execdpatch.elf > $(DISASM)/execdpatch_disasm.txt
	@$(OBJCOPY) -O binary execdpatch.elf execdpatch.bin
	@$(BIN2S) execdpatch.bin > execdpatch.s
	@mv -f execdpatch.s $(DEST)/execdpatch.s
	@rm *.bin
	@rm *.elf
	
build-sd:
	@echo Building SD Patch ...
	@$(CC) -O1 $(OPTS) -c base/base.S
	@$(CC) -O1 $(OPTS) -c sdgecko/sd.c
	@$(CC) -O1 $(OPTS) -c base/frag.c
	@$(CC) -O1 $(OPTS) -c base/igr.c
	@$(CC) -O1 $(OPTS) -c base/usbgecko.c
	@$(CC) -O1 $(OPTS) -c base/dvdqueue.c
	@$(CC) -O1 $(OPTS) -c base/dvdinterface.c
	@$(CC) -O1 $(OPTS) -c base/adp.c
	@$(CC) -O1 $(OPTS) -c base/Stream.c
	@$(CC) -O1 $(OPTS) -c base/__DSPHandler.S
	@$(CC) -O1 $(OPTS) -c base/memcpy.S
	@$(CC) -O1 $(OPTS) -o sd.elf -T base/base.ld -T base/common.ld base.o igr.o frag.o sd.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o memcpy.o
	@rm -rf *.o
	@$(OBJDUMP) -D sd.elf > $(DISASM)/sd_disasm.txt
	@$(OBJCOPY) -O binary sd.elf sd.bin
	@$(BIN2S) sd.bin > sd_final.s
	@mv -f sd_final.s $(DEST)/SlotAB-SD.s
	@rm *.bin
	@rm *.elf

build-sd-alt:
	@echo Building SD Alternate Patch ...
	@$(CC) -Os $(OPTS) -c alt/base.S
	@$(CC) -Os $(OPTS) -c alt/emulator.c
	@$(CC) -Os $(OPTS) -c alt/igr.c
	@$(CC) -Os $(OPTS) -c alt/blockdevice.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=2
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/os.c
	@$(CC) -Os $(OPTS) -o sd_alt.elf -T alt/base.ld -T base/common.ld -T base/os.ld base.o emulator.o igr.o blockdevice.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D sd_alt.elf > $(DISASM)/sd_alt_disasm.txt
	@$(OBJCOPY) -O binary sd_alt.elf sd_alt.bin
	@$(BIN2S) sd_alt.bin > sd_alt_final.s
	@mv -f sd_alt_final.s $(DEST)/SlotAB-SD_Alt.s
	@rm *.bin
	@rm *.elf

build-ideexi-v1:
	@echo Building IDE-EXI-v1 Patch ...
	@$(CC) -O1 $(OPTS) -c base/base.S
	@$(CC) -O1 $(OPTS) -c ide-exi/hddread.c -DDMA_READ=0
	@$(CC) -O1 $(OPTS) -c base/frag.c
	@$(CC) -O1 $(OPTS) -c base/igr.c
	@$(CC) -O1 $(OPTS) -c base/usbgecko.c
	@$(CC) -O1 $(OPTS) -c base/dvdqueue.c
	@$(CC) -O1 $(OPTS) -c base/dvdinterface.c
	@$(CC) -O1 $(OPTS) -c base/adp.c
	@$(CC) -O1 $(OPTS) -c base/Stream.c
	@$(CC) -O1 $(OPTS) -c base/__DSPHandler.S
	@$(CC) -O1 $(OPTS) -c base/memcpy.S
	@$(CC) -O1 $(OPTS) -o ideexi_v1.elf -T base/base.ld -T base/common.ld base.o igr.o frag.o hddread.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o memcpy.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_v1.elf > $(DISASM)/ideexi_v1_disasm.txt
	@$(OBJCOPY) -O binary ideexi_v1.elf ideexi_v1.bin
	@$(BIN2S) ideexi_v1.bin > ideexi_v1_final.s
	@mv -f ideexi_v1_final.s $(DEST)/SlotAB-IDEEXI_V1.s
	@rm *.bin
	@rm *.elf

build-ideexi-altv1:
	@echo Building IDE-EXI-v1 Alternate Patch ...
	@$(CC) -Os $(OPTS) -c alt/base.S
	@$(CC) -Os $(OPTS) -c alt/emulator.c
	@$(CC) -Os $(OPTS) -c alt/igr.c
	@$(CC) -Os $(OPTS) -c alt/blockdevice.c
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=0 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/os.c
	@$(CC) -Os $(OPTS) -o ideexi_altv1.elf -T alt/base.ld -T base/common.ld -T base/os.ld base.o emulator.o igr.o blockdevice.o hddread.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_altv1.elf > $(DISASM)/ideexi_altv1_disasm.txt
	@$(OBJCOPY) -O binary ideexi_altv1.elf ideexi_altv1.bin
	@$(BIN2S) ideexi_altv1.bin > ideexi_altv1_final.s
	@mv -f ideexi_altv1_final.s $(DEST)/SlotAB-IDEEXI_AltV1.s
	@rm *.bin
	@rm *.elf

build-ideexi-v2:
	@echo Building IDE-EXI-v2 Patch ...
	@$(CC) -O1 $(OPTS) -c base/base.S
	@$(CC) -O1 $(OPTS) -c ide-exi/hddread.c -DDMA_READ=1
	@$(CC) -O1 $(OPTS) -c base/frag.c
	@$(CC) -O1 $(OPTS) -c base/igr.c
	@$(CC) -O1 $(OPTS) -c base/usbgecko.c
	@$(CC) -O1 $(OPTS) -c base/dvdqueue.c
	@$(CC) -O1 $(OPTS) -c base/dvdinterface.c
	@$(CC) -O1 $(OPTS) -c base/adp.c
	@$(CC) -O1 $(OPTS) -c base/Stream.c
	@$(CC) -O1 $(OPTS) -c base/__DSPHandler.S
	@$(CC) -O1 $(OPTS) -c base/memcpy.S
	@$(CC) -O1 $(OPTS) -o ideexi_v2.elf -T base/base.ld -T base/common.ld base.o igr.o frag.o hddread.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o memcpy.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_v2.elf > $(DISASM)/ideexi_v2_disasm.txt
	@$(OBJCOPY) -O binary ideexi_v2.elf ideexi_v2.bin
	@$(BIN2S) ideexi_v2.bin > ideexi_v2_final.s
	@mv -f ideexi_v2_final.s $(DEST)/SlotAB-IDEEXI_V2.s
	@rm *.bin
	@rm *.elf

build-ideexi-altv2:
	@echo Building IDE-EXI-v2 Alternate Patch ...
	@$(CC) -Os $(OPTS) -c alt/base.S
	@$(CC) -Os $(OPTS) -c alt/emulator.c
	@$(CC) -Os $(OPTS) -c alt/igr.c
	@$(CC) -Os $(OPTS) -c alt/blockdevice.c
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=1 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/os.c
	@$(CC) -Os $(OPTS) -o ideexi_altv2.elf -T alt/base.ld -T base/common.ld -T base/os.ld base.o emulator.o igr.o blockdevice.o hddread.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_altv2.elf > $(DISASM)/ideexi_altv2_disasm.txt
	@$(OBJCOPY) -O binary ideexi_altv2.elf ideexi_altv2.bin
	@$(BIN2S) ideexi_altv2.bin > ideexi_altv2_final.s
	@mv -f ideexi_altv2_final.s $(DEST)/SlotAB-IDEEXI_AltV2.s
	@rm *.bin
	@rm *.elf

build-dvd:
	@echo Building DVD Patch ...
	@$(CC) -Os $(OPTS) -c alt/base.S
	@$(CC) -Os $(OPTS) -c alt/emulator.c -DDVD
	@$(CC) -Os $(OPTS) -c alt/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/os.c
	@$(CC) -Os $(OPTS) -o dvd.elf -T alt/base.ld -T base/common.ld -T base/os.ld base.o emulator.o igr.o dvd.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D dvd.elf > $(DISASM)/dvd_disasm.txt
	@$(OBJCOPY) -O binary dvd.elf dvd.bin
	@$(BIN2S) dvd.bin > dvd_final.s
	@mv -f dvd_final.s $(DEST)/DVDPatch.s
	@rm *.bin
	@rm *.elf

build-usb:
	@echo Building USBGecko Patch ...
	@$(CC) -Os $(OPTS) -c alt/base.S
	@$(CC) -Os $(OPTS) -c alt/emulator.c
	@$(CC) -Os $(OPTS) -c alt/igr.c
	@$(CC) -Os $(OPTS) -c usbgecko/usbgecko.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/os.c
	@$(CC) -Os $(OPTS) -o usbgecko.elf -T alt/base.ld -T base/common.ld -T base/os.ld base.o emulator.o igr.o usbgecko.o sd.o frag.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D usbgecko.elf > $(DISASM)/usb_disasm.txt
	@$(OBJCOPY) -O binary usbgecko.elf usbgecko.bin
	@$(BIN2S) usbgecko.bin > usbgecko_final.s
	@mv -f usbgecko_final.s $(DEST)/USBGeckoPatch.s
	@rm *.bin
	@rm *.elf

build-wkf:
	@echo Building WKF Patch ...
	@$(CC) -Os $(OPTS) -c alt/base.S
	@$(CC) -Os $(OPTS) -c alt/emulator.c -DWKF
	@$(CC) -Os $(OPTS) -c alt/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DPATCH_FRAGS=0
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/os.c
	@$(CC) -Os $(OPTS) -o wkf.elf -T alt/base.ld -T base/common.ld -T base/os.ld base.o emulator.o igr.o wkf.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D wkf.elf > $(DISASM)/wkf_disasm.txt
	@$(OBJCOPY) -O binary wkf.elf wkf.bin
	@$(BIN2S) wkf.bin > wkf.s
	@mv -f wkf.s $(DEST)/wkfPatch.s
	@rm *.bin
	@rm *.elf

build-bba:
	@echo Building BBA Patch ...
	@$(CC) -Os $(OPTS) -c alt/base.S
	@$(CC) -Os $(OPTS) -c alt/emulator.c -DBBA
	@$(CC) -Os $(OPTS) -c alt/igr.c
	@$(CC) -Os $(OPTS) -c bba/bba.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/os.c
	@$(CC) -Os $(OPTS) -o bba.elf -T alt/base.ld -T base/common.ld -T base/os.ld base.o emulator.o igr.o bba.o usbgecko.o sd.o frag.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D bba.elf > $(DISASM)/bba_disasm.txt
	@$(OBJCOPY) -O binary bba.elf bba.bin
	@$(BIN2S) bba.bin > bba.s
	@mv -f bba.s $(DEST)/BBAPatch.s
	@rm *.bin
	@rm *.elf
	
