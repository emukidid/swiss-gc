DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

OPTS	= -ffast-math -flto -fno-tree-loop-distribute-patterns -ffunction-sections -fdata-sections -fno-zero-initialized-in-bss -Wl,--gc-sections -Ibase

DEST    = ../swiss/source/patches

DISASM    = disassembly

.NOTPARALLEL:
all: stub sdpatch ideexipatch dvdpatch usbgeckopatch wkfpatch fsppatch gcloaderpatch

stub: stub.bin
sdpatch: sd.bin sd.dtk.bin
ideexipatch: ideexi-v1.bin ideexi-v1.dtk.bin ideexi-v2.bin ideexi-v2.dtk.bin
dvdpatch: dvd.bin dvd.card.bin
usbgeckopatch: usbgecko.bin
wkfpatch: wkf.bin wkf.card.bin wkf.dtk.bin
fsppatch: fsp.bin
gcloaderpatch: gcloader.bin gcloader.card.bin

clean:
	@rm -rf *.o
	@rm -rf $(DISASM)/*
	@-rmdir $(DISASM)
	@mkdir $(DISASM)

stub.bin:
	@echo Building Stub ...
	@$(CC) -Os $(OPTS) -c stub/crt0.S
	@$(CC) -Os $(OPTS) -c stub/main.c
	@$(CC) -Os $(OPTS) -c stub/asmfunc.S
	@$(CC) -Os $(OPTS) -c stub/mmcbbp.c
	@$(CC) -Os $(OPTS) -c stub/pff.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o stub.elf -T stub/stub.ld crt0.o main.o asmfunc.o mmcbbp.o pff.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D stub.elf > $(DISASM)/stub.txt
	@$(OBJCOPY) -O binary stub.elf stub.bin
	@$(BIN2S) stub.bin > $(DEST)/Stub.s
	@rm *.bin
	@rm *.elf

sd.bin:
	@echo Building SD Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DDVD_MATH -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=2
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o sd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o igr.o blockdevice.o sd.o sd_isr.o frag.o usbgecko.o os.o DVDMath.o floor.o sqrt.o
	@rm -rf *.o
	@$(OBJDUMP) -D sd.elf > $(DISASM)/sd.txt
	@$(OBJCOPY) -O binary sd.elf sd.bin
	@$(BIN2S) sd.bin > $(DEST)/SlotAB-SD.s
	@rm *.bin
	@rm *.elf

sd.dtk.bin:
	@echo Building SD Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DISR
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=3 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o sd.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o blockdevice.o sd.o sd_isr.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D sd.dtk.elf > $(DISASM)/sd.dtk.txt
	@$(OBJCOPY) -O binary sd.dtk.elf sd.dtk.bin
	@$(BIN2S) sd.dtk.bin > $(DEST)/SlotAB-SD.dtk.s
	@rm *.bin
	@rm *.elf

ideexi-v1.bin:
	@echo Building IDE-EXI-v1 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=0 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v1.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o blockdevice.o hddread.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi-v1.elf > $(DISASM)/ideexi-v1.txt
	@$(OBJCOPY) -O binary ideexi-v1.elf ideexi-v1.bin
	@$(BIN2S) ideexi-v1.bin > $(DEST)/SlotAB-IDEEXI_V1.s
	@rm *.bin
	@rm *.elf

ideexi-v1.dtk.bin:
	@echo Building IDE-EXI-v1 Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DDTK
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DDTK
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=0 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v1.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o blockdevice.o hddread.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi-v1.dtk.elf > $(DISASM)/ideexi-v1.dtk.txt
	@$(OBJCOPY) -O binary ideexi-v1.dtk.elf ideexi-v1.dtk.bin
	@$(BIN2S) ideexi-v1.dtk.bin > $(DEST)/SlotAB-IDEEXI_V1.dtk.s
	@rm *.bin
	@rm *.elf

ideexi-v2.bin:
	@echo Building IDE-EXI-v2 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=1 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v2.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o blockdevice.o hddread.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi-v2.elf > $(DISASM)/ideexi-v2.txt
	@$(OBJCOPY) -O binary ideexi-v2.elf ideexi-v2.bin
	@$(BIN2S) ideexi-v2.bin > $(DEST)/SlotAB-IDEEXI_V2.s
	@rm *.bin
	@rm *.elf

ideexi-v2.dtk.bin:
	@echo Building IDE-EXI-v2 Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c ide-exi/hddread.c -DDMA_READ=1 -DQUEUE_SIZE=3 -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v2.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o blockdevice.o hddread.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D ideexi-v2.dtk.elf > $(DISASM)/ideexi-v2.dtk.txt
	@$(OBJCOPY) -O binary ideexi-v2.dtk.elf ideexi-v2.dtk.bin
	@$(BIN2S) ideexi-v2.dtk.bin > $(DEST)/SlotAB-IDEEXI_V2.dtk.s
	@rm *.bin
	@rm *.elf

dvd.bin:
	@echo Building DVD Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DDI_PASSTHROUGH -DDVD
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o dvd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o dvd.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D dvd.elf > $(DISASM)/dvd.txt
	@$(OBJCOPY) -O binary dvd.elf dvd.bin
	@$(BIN2S) dvd.bin > $(DEST)/DVDPatch.s
	@rm *.bin
	@rm *.elf

dvd.card.bin:
	@echo Building DVD Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DCARD_EMULATOR -DDI_PASSTHROUGH -DDVD
	@$(CC) -Os $(OPTS) -c base/emulator_card.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o dvd.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o igr.o dvd.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D dvd.card.elf > $(DISASM)/dvd.card.txt
	@$(OBJCOPY) -O binary dvd.card.elf dvd.card.bin
	@$(BIN2S) dvd.card.bin > $(DEST)/DVDPatch.card.s
	@rm *.bin
	@rm *.elf

usbgecko.bin:
	@echo Building USBGecko Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c usbgecko/usbgecko.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o usbgecko.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o usbgecko.o sd.o frag.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D usbgecko.elf > $(DISASM)/usbgecko.txt
	@$(OBJCOPY) -O binary usbgecko.elf usbgecko.bin
	@$(BIN2S) usbgecko.bin > $(DEST)/USBGeckoPatch.s
	@rm *.bin
	@rm *.elf

wkf.bin:
	@echo Building WKF Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DWKF
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o wkf.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o wkf.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D wkf.elf > $(DISASM)/wkf.txt
	@$(OBJCOPY) -O binary wkf.elf wkf.bin
	@$(BIN2S) wkf.bin > $(DEST)/WKFPatch.s
	@rm *.bin
	@rm *.elf

wkf.card.bin:
	@echo Building WKF Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DCARD_EMULATOR -DWKF
	@$(CC) -Os $(OPTS) -c base/emulator_card.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o wkf.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o igr.o wkf.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D wkf.card.elf > $(DISASM)/wkf.card.txt
	@$(OBJCOPY) -O binary wkf.card.elf wkf.card.bin
	@$(BIN2S) wkf.card.bin > $(DEST)/WKFPatch.card.s
	@rm *.bin
	@rm *.elf

wkf.dtk.bin:
	@echo Building WKF Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DWKF
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c -DDTK -DQUEUE_SIZE=3
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o wkf.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o fifo.o igr.o wkf.o sd.o audio.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D wkf.dtk.elf > $(DISASM)/wkf.dtk.txt
	@$(OBJCOPY) -O binary wkf.dtk.elf wkf.dtk.bin
	@$(BIN2S) wkf.dtk.bin > $(DEST)/WKFPatch.dtk.s
	@rm *.bin
	@rm *.elf

fsp.bin:
	@echo Building FSP Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DBBA
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c bba/bba.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o fsp.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o igr.o bba.o usbgecko.o sd.o frag.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D fsp.elf > $(DISASM)/fsp.txt
	@$(OBJCOPY) -O binary fsp.elf fsp.bin
	@$(BIN2S) fsp.bin > $(DEST)/FSPPatch.s
	@rm *.bin
	@rm *.elf

gcloader.bin:
	@echo Building GCLoader Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DDI_PASSTHROUGH -DDVD_MATH
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o gcloader.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o igr.o dvd.o sd.o frag.o usbgecko.o os.o DVDMath.o floor.o sqrt.o
	@rm -rf *.o
	@$(OBJDUMP) -D gcloader.elf > $(DISASM)/gcloader.txt
	@$(OBJCOPY) -O binary gcloader.elf gcloader.bin
	@$(BIN2S) gcloader.bin > $(DEST)/GCLoaderPatch.s
	@rm *.bin
	@rm *.elf

gcloader.card.bin:
	@echo Building GCLoader Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DCARD_EMULATOR -DDI_PASSTHROUGH
	@$(CC) -Os $(OPTS) -c base/emulator_card.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DSINGLE_SECTOR=1
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/usbgecko.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o gcloader.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o igr.o dvd.o sd.o frag.o usbgecko.o os.o
	@rm -rf *.o
	@$(OBJDUMP) -D gcloader.card.elf > $(DISASM)/gcloader.card.txt
	@$(OBJCOPY) -O binary gcloader.card.elf gcloader.card.bin
	@$(BIN2S) gcloader.card.bin > $(DEST)/GCLoaderPatch.card.s
	@rm *.bin
	@rm *.elf
