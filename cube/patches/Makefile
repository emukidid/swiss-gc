DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

OPTS	= -ffast-math -flto -fno-tree-loop-distribute-patterns -ffunction-sections -fdata-sections -fno-zero-initialized-in-bss -Wl,--gc-sections -Ibase -Wno-address-of-packed-member -Wno-scalar-storage-order

DEST    = ../swiss/source/patches

DISASM    = disassembly

.NOTPARALLEL:
all: clean cpatches stub sdpatch ideexipatch dvdpatch usbgeckopatch wkfpatch fsppatch gcloaderpatch

stub: reboot.bin stub.bin
sdpatch: sd.bin sd.card.bin sd.dtk.bin
ideexipatch: ideexi-v1.bin ideexi-v1.card.bin ideexi-v1.dtk.bin ideexi-v2.bin ideexi-v2.card.bin ideexi-v2.dtk.bin
dvdpatch: dvd.bin dvd.card.bin
usbgeckopatch: usbgecko.bin
wkfpatch: wkf.bin wkf.card.bin wkf.dtk.bin
fsppatch: fsp.bin
gcloaderpatch: gcloader-v1.bin gcloader-v1.card.bin gcloader-v2.bin gcloader-v2.card.bin

clean:
	@rm -f *.bin *.elf *.o
	@rm -f $(DEST)/*.s
	@rm -fr $(DISASM)

cpatches:
	@echo Building C Patches ...
	@$(CC) -O3 $(OPTS) -c usbgecko/uart.c -DEXI=EXIEmuRegs
	@$(CC) -O3 $(OPTS) -o WriteUARTN.elf -e WriteUARTN -T base/common.ld -T base/dolphin/os.ld uart.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D WriteUARTN.elf > $(DISASM)/WriteUARTN.txt
	@$(OBJCOPY) -O binary WriteUARTN.elf WriteUARTN.bin
	@$(BIN2S) WriteUARTN.bin > $(DEST)/WriteUARTN.s
	@rm -f *.bin *.elf *.o

reboot.bin:
	@echo Building Reboot ...
	@$(CC) -Os $(OPTS) -c stub/crt0.S -DAPPLOADER
	@$(CC) -Os $(OPTS) -c stub/main.c
	@$(CC) -Os $(OPTS) -c stub/asmfunc.S -DAPPLOADER
	@$(CC) -Os $(OPTS) -c stub/mmcbbp.c
	@$(CC) -Os $(OPTS) -c stub/pff.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o reboot.elf -T stub/reboot.ld crt0.o main.o asmfunc.o mmcbbp.o pff.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D reboot.elf > $(DISASM)/reboot.txt
	@$(OBJCOPY) -O binary reboot.elf reboot.bin
	@$(BIN2S) reboot.bin > $(DEST)/Reboot.s
	@rm -f *.bin *.elf *.o

stub.bin:
	@echo Building Stub ...
	@$(CC) -Os $(OPTS) -c stub/crt0.S
	@$(CC) -Os $(OPTS) -c stub/main.c
	@$(CC) -Os $(OPTS) -c stub/asmfunc.S
	@$(CC) -Os $(OPTS) -c stub/mmcbbp.c
	@$(CC) -Os $(OPTS) -c stub/pff.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o stub.elf -T stub/stub.ld crt0.o main.o asmfunc.o mmcbbp.o pff.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D stub.elf > $(DISASM)/stub.txt
	@$(OBJCOPY) -O binary stub.elf stub.bin
	@$(BIN2S) stub.bin > $(DEST)/Stub.s
	@rm -f *.bin *.elf *.o

sd.bin:
	@echo Building SD Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=2
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDVD_MATH
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o sd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o igr.o blockdevice.o sd.o sd_isr.o uart.o os.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D sd.elf > $(DISASM)/sd.txt
	@$(OBJCOPY) -O binary sd.elf sd.bin
	@$(BIN2S) sd.bin > $(DEST)/SlotAB-SD.s
	@rm -f *.bin *.elf *.o

sd.card.bin:
	@echo Building SD Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o sd.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o igr.o blockdevice.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D sd.card.elf > $(DISASM)/sd.card.txt
	@$(OBJCOPY) -O binary sd.card.elf sd.card.bin
	@$(BIN2S) sd.card.bin > $(DEST)/SlotAB-SD.card.s
	@rm -f *.bin *.elf *.o

sd.dtk.bin:
	@echo Building SD Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DISR
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=3
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o sd.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o igr.o blockdevice.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D sd.dtk.elf > $(DISASM)/sd.dtk.txt
	@$(OBJCOPY) -O binary sd.dtk.elf sd.dtk.bin
	@$(BIN2S) sd.dtk.bin > $(DEST)/SlotAB-SD.dtk.s
	@rm -f *.bin *.elf *.o

ideexi-v1.bin:
	@echo Building IDE-EXI-v1 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=2
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDVD_MATH
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=0 -DISR_READ=1
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o ideexi-v1.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o igr.o blockdevice.o ata.o ata_isr.o uart.o os.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v1.elf > $(DISASM)/ideexi-v1.txt
	@$(OBJCOPY) -O binary ideexi-v1.elf ideexi-v1.bin
	@$(BIN2S) ideexi-v1.bin > $(DEST)/SlotAB-IDEEXI_V1.s
	@rm -f *.bin *.elf *.o

ideexi-v1.card.bin:
	@echo Building IDE-EXI-v1 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=0 -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v1.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o igr.o blockdevice.o ata.o ata_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v1.card.elf > $(DISASM)/ideexi-v1.card.txt
	@$(OBJCOPY) -O binary ideexi-v1.card.elf ideexi-v1.card.bin
	@$(BIN2S) ideexi-v1.card.bin > $(DEST)/SlotAB-IDEEXI_V1.card.s
	@rm -f *.bin *.elf *.o

ideexi-v1.dtk.bin:
	@echo Building IDE-EXI-v1 Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DISR
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=0 -DISR_READ=1 -DQUEUE_SIZE=3
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v1.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o igr.o blockdevice.o ata.o ata_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v1.dtk.elf > $(DISASM)/ideexi-v1.dtk.txt
	@$(OBJCOPY) -O binary ideexi-v1.dtk.elf ideexi-v1.dtk.bin
	@$(BIN2S) ideexi-v1.dtk.bin > $(DEST)/SlotAB-IDEEXI_V1.dtk.s
	@rm -f *.bin *.elf *.o

ideexi-v2.bin:
	@echo Building IDE-EXI-v2 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=2
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDVD_MATH
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=1 -DISR_READ=1
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S -DDMA
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o ideexi-v2.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o igr.o blockdevice.o ata.o ata_isr.o uart.o os.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v2.elf > $(DISASM)/ideexi-v2.txt
	@$(OBJCOPY) -O binary ideexi-v2.elf ideexi-v2.bin
	@$(BIN2S) ideexi-v2.bin > $(DEST)/SlotAB-IDEEXI_V2.s
	@rm -f *.bin *.elf *.o

ideexi-v2.card.bin:
	@echo Building IDE-EXI-v2 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=1 -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S -DDMA
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v2.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o igr.o blockdevice.o ata.o ata_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v2.card.elf > $(DISASM)/ideexi-v2.card.txt
	@$(OBJCOPY) -O binary ideexi-v2.card.elf ideexi-v2.card.bin
	@$(BIN2S) ideexi-v2.card.bin > $(DEST)/SlotAB-IDEEXI_V2.card.s
	@rm -f *.bin *.elf *.o

ideexi-v2.dtk.bin:
	@echo Building IDE-EXI-v2 Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DISR
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=1 -DISR_READ=1 -DQUEUE_SIZE=3
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S -DDMA
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o ideexi-v2.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o igr.o blockdevice.o ata.o ata_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v2.dtk.elf > $(DISASM)/ideexi-v2.dtk.txt
	@$(OBJCOPY) -O binary ideexi-v2.dtk.elf ideexi-v2.dtk.bin
	@$(BIN2S) ideexi-v2.dtk.bin > $(DEST)/SlotAB-IDEEXI_V2.dtk.s
	@rm -f *.bin *.elf *.o

dvd.bin:
	@echo Building DVD Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDI_PASSTHROUGH -DDVD -DISR
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o dvd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o igr.o dvd.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D dvd.elf > $(DISASM)/dvd.txt
	@$(OBJCOPY) -O binary dvd.elf dvd.bin
	@$(BIN2S) dvd.bin > $(DEST)/DVDPatch.s
	@rm -f *.bin *.elf *.o

dvd.card.bin:
	@echo Building DVD Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DDI_PASSTHROUGH -DDVD -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o dvd.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o igr.o dvd.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D dvd.card.elf > $(DISASM)/dvd.card.txt
	@$(OBJCOPY) -O binary dvd.card.elf dvd.card.bin
	@$(BIN2S) dvd.card.bin > $(DEST)/DVDPatch.card.s
	@rm -f *.bin *.elf *.o

usbgecko.bin:
	@echo Building USBGecko Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DISR -DUSB
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c usbgecko/usbgecko.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c usbgecko/usbgecko_isr.S -DMMC
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o usbgecko.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o igr.o usbgecko.o usbgecko_isr.o sd.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D usbgecko.elf > $(DISASM)/usbgecko.txt
	@$(OBJCOPY) -O binary usbgecko.elf usbgecko.bin
	@$(BIN2S) usbgecko.bin > $(DEST)/USBGeckoPatch.s
	@rm -f *.bin *.elf *.o

wkf.bin:
	@echo Building WKF Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DISR -DWKF
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o wkf.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o igr.o wkf.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D wkf.elf > $(DISASM)/wkf.txt
	@$(OBJCOPY) -O binary wkf.elf wkf.bin
	@$(BIN2S) wkf.bin > $(DEST)/WKFPatch.s
	@rm -f *.bin *.elf *.o

wkf.card.bin:
	@echo Building WKF Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DISR -DWKF
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o wkf.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o igr.o wkf.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D wkf.card.elf > $(DISASM)/wkf.card.txt
	@$(OBJCOPY) -O binary wkf.card.elf wkf.card.bin
	@$(BIN2S) wkf.card.bin > $(DEST)/WKFPatch.card.s
	@rm -f *.bin *.elf *.o

wkf.dtk.bin:
	@echo Building WKF Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DISR -DWKF
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c -DASYNC_READ -DQUEUE_SIZE=3
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o wkf.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o igr.o wkf.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D wkf.dtk.elf > $(DISASM)/wkf.dtk.txt
	@$(OBJCOPY) -O binary wkf.dtk.elf wkf.dtk.bin
	@$(BIN2S) wkf.dtk.bin > $(DEST)/WKFPatch.dtk.s
	@rm -f *.bin *.elf *.o

fsp.bin:
	@echo Building FSP Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DBBA -DISR
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c bba/bba.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o fsp.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o igr.o bba.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D fsp.elf > $(DISASM)/fsp.txt
	@$(OBJCOPY) -O binary fsp.elf fsp.bin
	@$(BIN2S) fsp.bin > $(DEST)/FSPPatch.s
	@rm -f *.bin *.elf *.o

gcloader-v1.bin:
	@echo Building GCLoader-v1 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDI_PASSTHROUGH -DGCODE -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=1
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ -DDVD_MATH -DGCODE
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o gcloader-v1.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o igr.o dvd.o sd.o sd_isr.o uart.o os.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v1.elf > $(DISASM)/gcloader-v1.txt
	@$(OBJCOPY) -O binary gcloader-v1.elf gcloader-v1.bin
	@$(BIN2S) gcloader-v1.bin > $(DEST)/GCLoaderPatch1.s
	@rm -f *.bin *.elf *.o

gcloader-v1.card.bin:
	@echo Building GCLoader-v1 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DDI_PASSTHROUGH -DGCODE -DISR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ -DGCODE
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o gcloader-v1.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o igr.o dvd.o sd.o sd_isr.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v1.card.elf > $(DISASM)/gcloader-v1.card.txt
	@$(OBJCOPY) -O binary gcloader-v1.card.elf gcloader-v1.card.bin
	@$(BIN2S) gcloader-v1.card.bin > $(DEST)/GCLoaderPatch1.card.s
	@rm -f *.bin *.elf *.o

gcloader-v2.bin:
	@echo Building GCLoader-v2 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DGCODE
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=1
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c gcloader/gcloader.c -DDVD_MATH
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o gcloader-v2.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o igr.o gcloader.o uart.o os.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v2.elf > $(DISASM)/gcloader-v2.txt
	@$(OBJCOPY) -O binary gcloader-v2.elf gcloader-v2.bin
	@$(BIN2S) gcloader-v2.bin > $(DEST)/GCLoaderPatch2.s
	@rm -f *.bin *.elf *.o

gcloader-v2.card.bin:
	@echo Building GCLoader-v2 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DGCODE
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/igr.c
	@$(CC) -Os $(OPTS) -c gcloader/gcloader.c -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -o gcloader-v2.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o igr.o gcloader.o uart.o os.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v2.card.elf > $(DISASM)/gcloader-v2.card.txt
	@$(OBJCOPY) -O binary gcloader-v2.card.elf gcloader-v2.card.bin
	@$(BIN2S) gcloader-v2.card.bin > $(DEST)/GCLoaderPatch2.card.s
	@rm -f *.bin *.elf *.o
