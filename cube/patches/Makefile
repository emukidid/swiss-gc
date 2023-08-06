DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

OPTS	= -ffast-math -flto -fipa-pta -fno-tree-loop-distribute-patterns -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--no-warn-rwx-segments -Wno-address-of-packed-member -Wno-scalar-storage-order -iquote base -iquote . -iquote ..

DEST    = ../swiss/source/patches

DISASM    = disassembly

.NOTPARALLEL:
all: clean codehandler cpatches stub sdpatch ideexipatch dvdpatch usbgeckopatch wkfpatch fsppatch gcloaderpatch

stub: reboot.bin stub.bin
sdpatch: sd.bin sd.card.bin sd.dtk.bin
ideexipatch: ideexi-v1.bin ideexi-v1.card.bin ideexi-v1.dtk.bin ideexi-v2.bin ideexi-v2.card.bin ideexi-v2.dtk.bin
dvdpatch: dvd.bin dvd.card.bin
usbgeckopatch: usbgecko.bin
wkfpatch: wkf.bin wkf.card.bin wkf.dtk.bin
fsppatch: fsp.bin fsp.dtk.bin fsp.eth.bin
gcloaderpatch: gcloader-v1.bin gcloader-v1.card.bin gcloader-v2.bin gcloader-v2.card.bin

clean:
	@rm -f *.bin *.elf *.o
	@rm -f $(DEST)/*.bin.s $(DEST)/*_bin.h
	@rm -fr $(DISASM)

codehandler:
	@echo Building Codehandler ...
	@$(CC) -nostartfiles -nodefaultlibs -nostdlib -o kenobigc.bin -T cheats/codehandler.ld cheats/codehandleronly.s
	@$(CC) -nostartfiles -nodefaultlibs -nostdlib -o kenobigc.dbg.bin -T cheats/codehandler.ld cheats/codehandler.s
	@$(BIN2S) -H $(DEST)/kenobigc_bin.h kenobigc.bin > $(DEST)/kenobigc.bin.s
	@$(BIN2S) -H $(DEST)/kenobigc_dbg_bin.h kenobigc.dbg.bin > $(DEST)/kenobigc.dbg.bin.s
	@rm -f *.bin

cpatches:
	@echo Building C Patches ...
	@$(CC) -O3 $(OPTS) -c base/memmove.S -DBACKWARDS_MEMCPY -o backwards_memcpy.o
	@$(CC) -O3 $(OPTS) -c base/memmove.S -DMEMCPY -o memcpy.o
	@$(CC) -O3 $(OPTS) -c base/os.c
	@$(CC) -O3 $(OPTS) -c base/pad.c
	@$(CC) -O3 $(OPTS) -c usbgecko/uart.c -DEXI=EXIEmuRegs
	@$(CC) -O3 $(OPTS) -o CallAlarmHandler.elf -e CallAlarmHandler -T base/common.ld -T base/dolphin/os.ld os.o
	@$(CC) -O3 $(OPTS) -o CheckStatus.elf -e CheckStatus -T base/common.ld -T base/dolphin/os.ld pad.o
	@$(CC) -O3 $(OPTS) -o WriteUARTN.elf -e WriteUARTN -T base/common.ld -T base/dolphin/os.ld uart.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D CallAlarmHandler.elf > $(DISASM)/CallAlarmHandler.txt
	@$(OBJDUMP) -D CheckStatus.elf > $(DISASM)/CheckStatus.txt
	@$(OBJDUMP) -D WriteUARTN.elf > $(DISASM)/WriteUARTN.txt
	@$(OBJCOPY) -O binary backwards_memcpy.o backwards_memcpy.bin
	@$(OBJCOPY) -O binary memcpy.o memcpy.bin
	@$(OBJCOPY) -O binary CallAlarmHandler.elf CallAlarmHandler.bin
	@$(OBJCOPY) -O binary CheckStatus.elf CheckStatus.bin
	@$(OBJCOPY) -O binary WriteUARTN.elf WriteUARTN.bin
	@$(BIN2S) -H $(DEST)/backwards_memcpy_bin.h backwards_memcpy.bin > $(DEST)/backwards_memcpy.bin.s
	@$(BIN2S) -H $(DEST)/memcpy_bin.h memcpy.bin > $(DEST)/memcpy.bin.s
	@$(BIN2S) -H $(DEST)/CallAlarmHandler_bin.h CallAlarmHandler.bin > $(DEST)/CallAlarmHandler.bin.s
	@$(BIN2S) -H $(DEST)/CheckStatus_bin.h CheckStatus.bin > $(DEST)/CheckStatus.bin.s
	@$(BIN2S) -H $(DEST)/WriteUARTN_bin.h WriteUARTN.bin > $(DEST)/WriteUARTN.bin.s
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
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents reboot.elf reboot.bin
	@truncate -s %32 reboot.bin
	@$(BIN2S) -H $(DEST)/reboot_bin.h reboot.bin > $(DEST)/reboot.bin.s
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
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents stub.elf stub.bin
	@$(BIN2S) -H $(DEST)/stub_bin.h stub.bin > $(DEST)/stub.bin.s
	@rm -f *.bin *.elf *.o

sd.bin:
	@echo Building SD Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=2
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDVD_MATH
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o sd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o interrupt.o ipl.o blockdevice.o sd.o sd_isr.o uart.o os.o memcpy.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D sd.elf > $(DISASM)/sd.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents sd.elf sd.bin
	@$(BIN2S) -H $(DEST)/sd_bin.h sd.bin > $(DEST)/sd.bin.s
	@rm -f *.bin *.elf *.o

sd.card.bin:
	@echo Building SD Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o sd.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o interrupt.o ipl.o blockdevice.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D sd.card.elf > $(DISASM)/sd.card.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents sd.card.elf sd.card.bin
	@$(BIN2S) -H $(DEST)/sd_card_bin.h sd.card.bin > $(DEST)/sd.card.bin.s
	@rm -f *.bin *.elf *.o

sd.dtk.bin:
	@echo Building SD Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=3 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o sd.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o interrupt.o ipl.o blockdevice.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D sd.dtk.elf > $(DISASM)/sd.dtk.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents sd.dtk.elf sd.dtk.bin
	@$(BIN2S) -H $(DEST)/sd_dtk_bin.h sd.dtk.bin > $(DEST)/sd.dtk.bin.s
	@rm -f *.bin *.elf *.o

ideexi-v1.bin:
	@echo Building IDE-EXI-v1 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=2
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDVD_MATH
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=0 -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o ideexi-v1.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o interrupt.o ipl.o blockdevice.o ata.o ata_isr.o uart.o os.o memcpy.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v1.elf > $(DISASM)/ideexi-v1.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents ideexi-v1.elf ideexi-v1.bin
	@$(BIN2S) -H $(DEST)/ideexi_v1_bin.h ideexi-v1.bin > $(DEST)/ideexi-v1.bin.s
	@rm -f *.bin *.elf *.o

ideexi-v1.card.bin:
	@echo Building IDE-EXI-v1 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=0 -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o ideexi-v1.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o interrupt.o ipl.o blockdevice.o ata.o ata_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v1.card.elf > $(DISASM)/ideexi-v1.card.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents ideexi-v1.card.elf ideexi-v1.card.bin
	@$(BIN2S) -H $(DEST)/ideexi_v1_card_bin.h ideexi-v1.card.bin > $(DEST)/ideexi-v1.card.bin.s
	@rm -f *.bin *.elf *.o

ideexi-v1.dtk.bin:
	@echo Building IDE-EXI-v1 Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=0 -DISR_READ=1 -DQUEUE_SIZE=3 -DWRITE=0
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o ideexi-v1.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o interrupt.o ipl.o blockdevice.o ata.o ata_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v1.dtk.elf > $(DISASM)/ideexi-v1.dtk.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents ideexi-v1.dtk.elf ideexi-v1.dtk.bin
	@$(BIN2S) -H $(DEST)/ideexi_v1_dtk_bin.h ideexi-v1.dtk.bin > $(DEST)/ideexi-v1.dtk.bin.s
	@rm -f *.bin *.elf *.o

ideexi-v2.bin:
	@echo Building IDE-EXI-v2 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=2
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDVD_MATH
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=1 -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S -DDMA
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o ideexi-v2.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o interrupt.o ipl.o blockdevice.o ata.o ata_isr.o uart.o os.o memcpy.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v2.elf > $(DISASM)/ideexi-v2.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents ideexi-v2.elf ideexi-v2.bin
	@$(BIN2S) -H $(DEST)/ideexi_v2_bin.h ideexi-v2.bin > $(DEST)/ideexi-v2.bin.s
	@rm -f *.bin *.elf *.o

ideexi-v2.card.bin:
	@echo Building IDE-EXI-v2 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=1 -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S -DDMA
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o ideexi-v2.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o interrupt.o ipl.o blockdevice.o ata.o ata_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v2.card.elf > $(DISASM)/ideexi-v2.card.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents ideexi-v2.card.elf ideexi-v2.card.bin
	@$(BIN2S) -H $(DEST)/ideexi_v2_card_bin.h ideexi-v2.card.bin > $(DEST)/ideexi-v2.card.bin.s
	@rm -f *.bin *.elf *.o

ideexi-v2.dtk.bin:
	@echo Building IDE-EXI-v2 Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c base/blockdevice.c -DASYNC_READ -DDTK
	@$(CC) -Os $(OPTS) -c ide-exi/ata.c -DDMA_READ=1 -DISR_READ=1 -DQUEUE_SIZE=3 -DWRITE=0
	@$(CC) -Os $(OPTS) -c ide-exi/ata_isr.S -DDMA
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o ideexi-v2.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o interrupt.o ipl.o blockdevice.o ata.o ata_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D ideexi-v2.dtk.elf > $(DISASM)/ideexi-v2.dtk.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents ideexi-v2.dtk.elf ideexi-v2.dtk.bin
	@$(BIN2S) -H $(DEST)/ideexi_v2_dtk_bin.h ideexi-v2.dtk.bin > $(DEST)/ideexi-v2.dtk.bin.s
	@rm -f *.bin *.elf *.o

dvd.bin:
	@echo Building DVD Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDI_PASSTHROUGH -DDVD
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o dvd.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o interrupt.o ipl.o dvd.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D dvd.elf > $(DISASM)/dvd.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents dvd.elf dvd.bin
	@$(BIN2S) -H $(DEST)/dvd_bin.h dvd.bin > $(DEST)/dvd.bin.s
	@rm -f *.bin *.elf *.o

dvd.card.bin:
	@echo Building DVD Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DDI_PASSTHROUGH -DDVD
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o dvd.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o interrupt.o ipl.o dvd.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D dvd.card.elf > $(DISASM)/dvd.card.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents dvd.card.elf dvd.card.bin
	@$(BIN2S) -H $(DEST)/dvd_card_bin.h dvd.card.bin > $(DEST)/dvd.card.bin.s
	@rm -f *.bin *.elf *.o

usbgecko.bin:
	@echo Building USBGecko Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DUSB
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c usbgecko/usbgecko.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c usbgecko/usbgecko_isr.S
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o usbgecko.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o interrupt.o ipl.o usbgecko.o usbgecko_isr.o sd.o sd_isr.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D usbgecko.elf > $(DISASM)/usbgecko.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents usbgecko.elf usbgecko.bin
	@$(BIN2S) -H $(DEST)/usbgecko_bin.h usbgecko.bin > $(DEST)/usbgecko.bin.s
	@rm -f *.bin *.elf *.o

wkf.bin:
	@echo Building WKF Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DWKF
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o wkf.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o interrupt.o ipl.o wkf.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D wkf.elf > $(DISASM)/wkf.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents wkf.elf wkf.bin
	@$(BIN2S) -H $(DEST)/wkf_bin.h wkf.bin > $(DEST)/wkf.bin.s
	@rm -f *.bin *.elf *.o

wkf.card.bin:
	@echo Building WKF Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DWKF
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o wkf.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o interrupt.o ipl.o wkf.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D wkf.card.elf > $(DISASM)/wkf.card.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents wkf.card.elf wkf.card.bin
	@$(BIN2S) -H $(DEST)/wkf_card_bin.h wkf.card.bin > $(DEST)/wkf.card.bin.s
	@rm -f *.bin *.elf *.o

wkf.dtk.bin:
	@echo Building WKF Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDTK -DWKF
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c wkf/wkf.c -DASYNC_READ -DQUEUE_SIZE=3
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o wkf.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o interrupt.o ipl.o wkf.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D wkf.dtk.elf > $(DISASM)/wkf.dtk.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents wkf.dtk.elf wkf.dtk.bin
	@$(BIN2S) -H $(DEST)/wkf_dtk_bin.h wkf.dtk.bin > $(DEST)/wkf.dtk.bin.s
	@rm -f *.bin *.elf *.o

fsp.bin:
	@echo Building FSP Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DBBA -DNO_VIDEO
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c bba/bba.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/setjmp.S
	@$(CC) -Os $(OPTS) -o fsp.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o frag.o interrupt.o ipl.o bba.o sd.o sd_isr.o uart.o os.o memcpy.o setjmp.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D fsp.elf > $(DISASM)/fsp.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents fsp.elf fsp.bin
	@$(BIN2S) -H $(DEST)/fsp_bin.h fsp.bin > $(DEST)/fsp.bin.s
	@rm -f *.bin *.elf *.o

fsp.dtk.bin:
	@echo Building FSP Patch + DTK ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DBBA -DDTK -DNO_VIDEO
	@$(CC) -Os $(OPTS) -c base/audio.c
	@$(CC) -Os $(OPTS) -c base/fifo.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c bba/bba.c -DASYNC_READ -DQUEUE_SIZE=3
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/setjmp.S
	@$(CC) -Os $(OPTS) -o fsp.dtk.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o audio.o fifo.o frag.o interrupt.o ipl.o bba.o uart.o os.o memcpy.o setjmp.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D fsp.dtk.elf > $(DISASM)/fsp.dtk.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents fsp.dtk.elf fsp.dtk.bin
	@$(BIN2S) -H $(DEST)/fsp_dtk_bin.h fsp.dtk.bin > $(DEST)/fsp.dtk.bin.s
	@rm -f *.bin *.elf *.o

fsp.eth.bin:
	@echo Building FSP Patch + ETH ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DBBA -DETH_EMULATOR -DNO_VIDEO
	@$(CC) -Os $(OPTS) -c base/emulator_eth.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c bba/bba.c -DASYNC_READ -DETH_EMULATOR
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/setjmp.S
	@$(CC) -Os $(OPTS) -o fsp.eth.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_eth.o frag.o interrupt.o ipl.o bba.o uart.o os.o memcpy.o setjmp.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D fsp.eth.elf > $(DISASM)/fsp.eth.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents fsp.eth.elf fsp.eth.bin
	@$(BIN2S) -H $(DEST)/fsp_eth_bin.h fsp.eth.bin > $(DEST)/fsp.eth.bin.s
	@rm -f *.bin *.elf *.o

gcloader-v1.bin:
	@echo Building GCLoader-v1 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DDI_PASSTHROUGH -DGCODE
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=1
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ -DDVD_MATH -DGCODE
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DWRITE=0
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o gcloader-v1.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o interrupt.o ipl.o dvd.o sd.o sd_isr.o uart.o os.o memcpy.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v1.elf > $(DISASM)/gcloader-v1.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents gcloader-v1.elf gcloader-v1.bin
	@$(BIN2S) -H $(DEST)/gcloader_v1_bin.h gcloader-v1.bin > $(DEST)/gcloader-v1.bin.s
	@rm -f *.bin *.elf *.o

gcloader-v1.card.bin:
	@echo Building GCLoader-v1 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DDI_PASSTHROUGH -DGCODE
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1 -DDIRECT_DISC
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c dvd/dvd.c -DASYNC_READ -DGCODE
	@$(CC) -Os $(OPTS) -c sdgecko/sd.c -DISR_READ=1 -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c sdgecko/sd_isr.S
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o gcloader-v1.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o interrupt.o ipl.o dvd.o sd.o sd_isr.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v1.card.elf > $(DISASM)/gcloader-v1.card.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents gcloader-v1.card.elf gcloader-v1.card.bin
	@$(BIN2S) -H $(DEST)/gcloader_v1_card_bin.h gcloader-v1.card.bin > $(DEST)/gcloader-v1.card.bin.s
	@rm -f *.bin *.elf *.o

gcloader-v2.bin:
	@echo Building GCLoader-v2 Patch ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DGCODE
	@$(CC) -Os $(OPTS) -c base/emulator_dvd.c -DREAD_SPEED_TIER=1
	@$(CC) -Os $(OPTS) -c base/DVDMath.c
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c gcloader/gcloader.c -DDVD_MATH
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -c base/floor.S
	@$(CC) -Os $(OPTS) -c base/sqrt.S
	@$(CC) -Os $(OPTS) -o gcloader-v2.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_dvd.o DVDMath.o frag.o interrupt.o ipl.o gcloader.o uart.o os.o memcpy.o floor.o sqrt.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v2.elf > $(DISASM)/gcloader-v2.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents gcloader-v2.elf gcloader-v2.bin
	@$(BIN2S) -H $(DEST)/gcloader_v2_bin.h gcloader-v2.bin > $(DEST)/gcloader-v2.bin.s
	@rm -f *.bin *.elf *.o

gcloader-v2.card.bin:
	@echo Building GCLoader-v2 Patch + CARD ...
	@$(CC) -Os $(OPTS) -c base/base.S
	@$(CC) -Os $(OPTS) -c base/emulator.c -DASYNC_READ -DCARD_EMULATOR -DGCODE
	@$(CC) -Os $(OPTS) -c base/emulator_card.c -DASYNC_READ
	@$(CC) -Os $(OPTS) -c base/frag.c -DDEVICE_PATCHES=1
	@$(CC) -Os $(OPTS) -c base/interrupt.c
	@$(CC) -Os $(OPTS) -c base/ipl.c
	@$(CC) -Os $(OPTS) -c gcloader/gcloader.c -DQUEUE_SIZE=4
	@$(CC) -Os $(OPTS) -c usbgecko/uart.c
	@$(CC) -Os $(OPTS) -c base/dolphin/os.c
	@$(CC) -Os $(OPTS) -c base/memcpy.S
	@$(CC) -Os $(OPTS) -o gcloader-v2.card.elf -T base/base.ld -T base/common.ld -T base/dolphin/os.ld base.o emulator.o emulator_card.o frag.o interrupt.o ipl.o gcloader.o uart.o os.o memcpy.o
	@mkdir -p $(DISASM)
	@$(OBJDUMP) -D gcloader-v2.card.elf > $(DISASM)/gcloader-v2.card.txt
	@$(OBJCOPY) -O binary --set-section-flags .bss.*=alloc,load,contents gcloader-v2.card.elf gcloader-v2.card.bin
	@$(BIN2S) -H $(DEST)/gcloader_v2_card_bin.h gcloader-v2.card.bin > $(DEST)/gcloader-v2.card.bin.s
	@rm -f *.bin *.elf *.o
