DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

OPTS	= -ffast-math -flto -ffunction-sections -fdata-sections -fno-zero-initialized-in-bss -Wl,--gc-sections

DEST    = ../swiss/source/patches

DISASM    = disassembly

.NOTPARALLEL:
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
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DISR
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o sd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o blockdevice.o sd.o sd_isr.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D sd.elf > $(DISASM)/sd_disasm.txt
	@$(OBJCOPY) -O binary sd.elf sd.bin
	@$(BIN2S) sd.bin > $(DEST)/SlotAB-SD.s
	@rm *.bin
	@rm *.elf

build-ideexi-v1:
	@echo Building IDE-EXI-v1 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DDTK
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=0 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi_v1.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o blockdevice.o hddread.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_v1.elf > $(DISASM)/ideexi_v1_disasm.txt
	@$(OBJCOPY) -O binary ideexi_v1.elf ideexi_v1.bin
	@$(BIN2S) ideexi_v1.bin > $(DEST)/SlotAB-IDEEXI_V1.s
	@rm *.bin
	@rm *.elf

build-ideexi-v2:
	@echo Building IDE-EXI-v2 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=1 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi_v2.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o blockdevice.o hddread.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi_v2.elf > $(DISASM)/ideexi_v2_disasm.txt
	@$(OBJCOPY) -O binary ideexi_v2.elf ideexi_v2.bin
	@$(BIN2S) ideexi_v2.bin > $(DEST)/SlotAB-IDEEXI_V2.s
	@rm *.bin
	@rm *.elf

build-dvd:
	@echo Building DVD Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DDVD
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DPATCH_FRAGS=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o dvd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o dvd.o sd.o frag.o usbgecko.o os.o DVDMath.o sqrt.o
	@rm -rf *.o
	@$(OBJDUMP) -D dvd.elf > $(DISASM)/dvd_disasm.txt
	@$(OBJCOPY) -O binary dvd.elf dvd.bin
	@$(BIN2S) dvd.bin > $(DEST)/DVDPatch.s
	@rm *.bin
	@rm *.elf

build-usb:
	@echo Building USBGecko Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c usbgecko/usbgecko.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DPATCH_FRAGS=1
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o usbgecko.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o usbgecko.o sd.o frag.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D usbgecko.elf > $(DISASM)/usb_disasm.txt
	@$(OBJCOPY) -O binary usbgecko.elf usbgecko.bin
	@$(BIN2S) usbgecko.bin > $(DEST)/USBGeckoPatch.s
	@rm *.bin
	@rm *.elf

build-wkf:
	@echo Building WKF Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DWKF
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DPATCH_FRAGS=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o wkf.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o wkf.o sd.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D wkf.elf > $(DISASM)/wkf_disasm.txt
	@$(OBJCOPY) -O binary wkf.elf wkf.bin
	@$(BIN2S) wkf.bin > $(DEST)/WKFPatch.s
	@rm *.bin
	@rm *.elf

build-bba:
	@echo Building BBA Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DBBA
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c bba/bba.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DPATCH_FRAGS=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o bba.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o bba.o usbgecko.o sd.o frag.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D bba.elf > $(DISASM)/bba_disasm.txt
	@$(OBJCOPY) -O binary bba.elf bba.bin
	@$(BIN2S) bba.bin > $(DEST)/BBAPatch.s
	@rm *.bin
	@rm *.elf
