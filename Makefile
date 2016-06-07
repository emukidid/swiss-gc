#---------------------------------------------------------------------------------
REVISION  = git rev-list HEAD | wc -l
SVN_REVISION = swiss_r$(shell $(REVISION))

DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DIR)/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

DIST          = dist
SOURCES       = cube
BUILDTOOLS    = buildtools
BASE          = $(SOURCES)/patches/base
DVD_SOURCES   = $(SOURCES)/patches/dvd
HDD_SOURCES   = $(SOURCES)/patches/ide-exi
SD_SOURCES    = $(SOURCES)/patches/sdgecko
USB_SOURCES   = $(SOURCES)/patches/usbgecko
WKF_SOURCES   = $(SOURCES)/patches/wkf
AR_SOURCES    = $(SOURCES)/actionreplay

BUILT_PATCHES = patches
GECKOSERVER   = pc/usbgecko

#------------------------------------------------------------------

#all: clean update build build-AR package
#rev: clean user build build-AR package
all: clean compile build recovery-iso build-AR build-geckoserver package

clean:
	@rm -rf $(DIST)
	@cd $(SOURCES)/swiss && make clean
	@cd $(GECKOSERVER) && make clean

#------------------------------------------------------------------
compile: # compile
	@cd $(SOURCES)/swiss && make

#------------------------------------------------------------------

build:
	# create initial DIR structure and various DOLs 
	@mkdir $(DIST)
	@mkdir $(DIST)/DOL
	@mkdir $(DIST)/DOL/Viper
	@mkdir $(DIST)/ISO
	@mkdir $(DIST)/ISO/DOL
	@mkdir $(DIST)/WiikeyFusion
	@mkdir $(DIST)/WiikeyFusion/RecoveryISO
	@mkdir $(DIST)/ActionReplay
	@cp $(SOURCES)/swiss/swiss.dol $(DIST)/DOL/$(SVN_REVISION).dol
	@cp $(SOURCES)/swiss/swiss.elf $(DIST)/DOL/$(SVN_REVISION).elf
	@$(BUILDTOOLS)/dollz3 $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/DOL/Viper/$(SVN_REVISION)\-lz-viper.dol -m -v
	@$(BUILDTOOLS)/dollz3 $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/DOL/$(SVN_REVISION)\-compressed.dol -m
	@cp $(DIST)/DOL/$(SVN_REVISION)\-compressed.dol $(DIST)/ActionReplay/swiss-lz.dol
	@cp $(DIST)/DOL/$(SVN_REVISION)\-compressed.dol $(DIST)/ISO/DOL/$(SVN_REVISION)\-compressed.dol
	# make ISOs and WKF firmware
	# NTSC
	@mkisofs -R -J -G $(BUILDTOOLS)/iso/eltorito-u.hdr -no-emul-boot -b $(SVN_REVISION)-compressed.dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-u)".iso $(DIST)/ISO/DOL/
	## NTSC-J
	@mkisofs -R -J -G $(BUILDTOOLS)/iso/eltorito-j.hdr -no-emul-boot -b $(SVN_REVISION)-compressed.dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-j)".iso $(DIST)/ISO/DOL/
	# PAL
	@mkisofs -R -J -G $(BUILDTOOLS)/iso/eltorito-e.hdr -no-emul-boot -b $(SVN_REVISION)-compressed.dol -o $(DIST)/ISO/$(SVN_REVISION)"(pal)".iso $(DIST)/ISO/DOL/
	@cp $(BUILDTOOLS)/wkf/wkf_menu.fzn $(SVN_REVISION).fzn
	@dd if=$(DIST)/ISO/$(SVN_REVISION)"(pal)".iso of=$(SVN_REVISION).fzn conv=notrunc
	@mv $(SVN_REVISION).fzn $(DIST)/WiikeyFusion
	@cp $(BUILDTOOLS)/wkf/autoboot.fzn.fw $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn.fw
	@rm $(DIST)/ISO/DOL/$(SVN_REVISION)\-compressed.dol
	@rmdir $(DIST)/ISO/DOL

#------------------------------------------------------------------

recovery-iso:
	@cp $(BUILDTOOLS)/wkf/recovery_bootloader.iso $(DIST)/WiikeyFusion/RecoveryISO/$(SVN_REVISION)"_Recovery".iso
	# crop swiss iso
	@dd if=$(DIST)/ISO/$(SVN_REVISION)"(pal)".iso of=$(DIST)/WiikeyFusion/RecoveryISO/pal.iso skip=65536 bs=1
	# merge bootloader and swiss
	@dd if=$(DIST)/WiikeyFusion/RecoveryISO/pal.iso of=$(DIST)/WiikeyFusion/RecoveryISO/$(SVN_REVISION)"_Recovery".iso bs=1 seek=65536
	@rm -rf $(DIST)/WiikeyFusion/RecoveryISO/pal.iso

#------------------------------------------------------------------

package:   # create distribution package
	@mkdir $(SVN_REVISION)
	@mv $(DIST)/DOL $(SVN_REVISION)
	@mv $(DIST)/ISO $(SVN_REVISION)
	@mv $(DIST)/WiikeyFusion $(SVN_REVISION)
	@mv $(DIST)/ActionReplay $(SVN_REVISION)
	@mv $(DIST)/USBGeckoRemoteServer $(SVN_REVISION)
	@find ./$(SVN_REVISION) -type f -print0 | xargs -0 md5sum > $(SVN_REVISION).md5
	@mv $(SVN_REVISION).md5 $(SVN_REVISION)
	@git log -n 4 > $(SVN_REVISION)-changelog.txt
	@sed -i "s/emukidid <emukidid@gmail.com>/emu_kidid/g" $(SVN_REVISION)-changelog.txt
	@mv $(SVN_REVISION)-changelog.txt $(SVN_REVISION)
	@cp $(BUILDTOOLS)/SWISS_FILE_DESCRIPTIONS.txt $(SVN_REVISION)
	@7z a -m0=LZMA -m1=LZMA $(SVN_REVISION).7z $(SVN_REVISION)
	@7z t $(SVN_REVISION).7z $(SVN_REVISION)/*

#------------------------------------------------------------------

build-AR: # make ActionReplay
	@$(BIN2S) $(DIST)/ActionReplay/swiss-lz.dol > $(AR_SOURCES)/swiss-lz.s
	@$(CC) -O2 -c $(AR_SOURCES)/startup.s -o $(AR_SOURCES)/startup.o
	@$(CC) -O2 -c $(AR_SOURCES)/swiss-lz.s -o $(AR_SOURCES)/swiss-lz.o
	@$(CC) -O2 -c $(AR_SOURCES)/main.c -o $(AR_SOURCES)/main.o
	@$(LD) -o $(AR_SOURCES)/sdloader.elf $(AR_SOURCES)/startup.o $(AR_SOURCES)/main.o $(AR_SOURCES)/swiss-lz.o --section-start .text=0x81700000
	@$(OBJCOPY) -O binary $(AR_SOURCES)/sdloader.elf $(DIST)/ActionReplay/SDLOADER.BIN
	@rm -f $(AR_SOURCES)/*.o $(AR_SOURCES)/*.elf $(DIST)/ActionReplay/*swiss-lz.dol $(AR_SOURCES)/swiss-lz.s

#------------------------------------------------------------------


build-geckoserver:
	@cd $(GECKOSERVER) && make
	@mkdir $(DIST)/USBGeckoRemoteServer
	@mv $(GECKOSERVER)/swissserver* $(DIST)/USBGeckoRemoteServer/
	


#------------------------------------------------------------------

# Make individual patches
build-dvd: #clean
	@mkdir $(BUILT_PATCHES)
	@$(CC) -O2 -c $(DVD_SOURCES)/base.S
	@$(CC) -O2 -c $(DVD_SOURCES)/dvd.c
	@$(CC) -O2 -c $(SD_SOURCES)/sd.c
	@$(CC) -O2 -c $(BASE)/frag.c
	@$(CC) -O2 -c $(BASE)/usbgecko.c
	@$(CC) -O2 -c $(BASE)/dvdqueue.c
	@$(LD) -o dvd.elf base.o frag.o sd.o dvd.o dvdqueue.o usbgecko.o --section-start .text=0x80001800
	@$(OBJDUMP) -D dvd.elf > $(BUILT_PATCHES)/dvd_disasm.txt
	@$(OBJCOPY) -O binary dvd.elf dvd.bin
	@$(BIN2S) dvd.bin > dvd_final.s
	@mv dvd_final.s $(BUILT_PATCHES)/DVDPatch.s
	@rm -rf *.o *.bin *.elf

build-hdd: clean
	@mkdir $(BUILT_PATCHES)
	@$(CC) -O2 -c $(BASE)/base.S
	@$(CC) -O2 -c $(HDD_SOURCES)/hddread.c
	@$(CC) -O2 -c $(BASE)/frag.c
	@$(CC) -O2 -c $(BASE)/usbgecko.c
	@$(CC) -O2 -c $(BASE)/dvdqueue.c
	@$(CC) -O2 -c $(BASE)/dvdinterface.c
	@$(CC) -O2 -c $(BASE)/adp.c
	@$(CC) -O1 -c $(BASE)/Stream.c
	@$(CC) -O2 -c $(BASE)/__DSPHandler.S
	@$(CC) -O1 -c $(BASE)/appldr_start.S
	@$(CC) -O1 -c $(BASE)/appldr.c
	@$(LD) -o hdd.elf base.o frag.o hddread.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o --section-start .text=0x80001000
	@$(OBJDUMP) -D hdd.elf > $(BUILT_PATCHES)/hdd_disasm.txt
	@$(OBJCOPY) -O binary hdd.elf hdd.bin
	@$(BIN2S) hdd.bin > hdd_final.s
	@mv hdd_final.s $(BUILT_PATCHES)/SlotAB-HDD.s
	@rm -rf *.o *.bin *.elf

build-sd: clean
	@mkdir $(BUILT_PATCHES)
	@$(CC) -O2 -c $(BASE)/base.S
	@$(CC) -O2 -c $(SD_SOURCES)/sd.c
	@$(CC) -O2 -c $(BASE)/frag.c
	@$(CC) -O2 -c $(BASE)/usbgecko.c
	@$(CC) -O2 -c $(BASE)/dvdqueue.c
	@$(CC) -O2 -c $(BASE)/dvdinterface.c
	@$(CC) -O2 -c $(BASE)/adp.c
	@$(CC) -O1 -c $(BASE)/Stream.c
	@$(CC) -O2 -c $(BASE)/__DSPHandler.S
	@$(CC) -O1 -c $(BASE)/appldr_start.S
	@$(CC) -O1 -c $(BASE)/appldr.c
	@$(LD) -o sd.elf base.o frag.o sd.o dvdinterface.o dvdqueue.o usbgecko.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o --section-start .text=0x80001000
	@$(OBJDUMP) -D sd.elf > $(BUILT_PATCHES)/sd_disasm.txt
	@$(OBJCOPY) -O binary sd.elf sd.bin
	@$(BIN2S) sd.bin > sd_final.s
	@mv sd_final.s $(BUILT_PATCHES)/SlotAB-SD.s
	@rm -rf *.o *.bin *.elf

build-usb: clean
	@mkdir $(BUILT_PATCHES)
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c $(BASE)/base.S
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c $(USB_SOURCES)/usbgecko.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c $(BASE)/frag.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c $(BASE)/dvdqueue.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c $(BASE)/dvdinterface.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c $(BASE)/adp.c
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c $(BASE)/Stream.c
	@$(CC) -O2 -ffunction-sections -mmultiple -mstring -c $(BASE)/__DSPHandler.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c $(BASE)/appldr_start.S
	@$(CC) -O1 -ffunction-sections -mmultiple -mstring -c $(BASE)/appldr.c
	@$(LD) -o usbgecko.elf base.o usbgecko.o dvdqueue.o dvdinterface.o adp.o Stream.o __DSPHandler.o appldr_start.o appldr.o frag.o --section-start .text=0x80001000 --gc-sections
	@$(OBJDUMP) -D usbgecko.elf > $(BUILT_PATCHES)/usb_disasm.txt
	@$(OBJCOPY) -O binary usbgecko.elf usbgecko.bin
	@$(BIN2S) usbgecko.bin > usbgecko_final.s
	@mv usbgecko_final.s $(BUILT_PATCHES)/USBGeckoRead.s
	@rm -rf *.o *.bin *.elf

build-wkf: clean
	@mkdir $(BUILT_PATCHES)
	@$(CC) -O2 -c $(WKF_SOURCES)/base.S
	@$(CC) -O2 -c $(WKF_SOURCES)/wkf.c
	@$(CC) -O2 -DDEBUG -c $(BASE)/usbgecko.c
	@$(LD) -o wkf.elf base.o wkf.o usbgecko.o --section-start .text=0x80001800
	@$(OBJDUMP) -D wkf.elf > $(BUILT_PATCHES)/wkf_disasm.txt
	@$(OBJCOPY) -O binary wkf.elf wkf.bin
	@$(BIN2S) wkf.bin > wkfPatch.s
	@mv wkfPatch.s $(BUILT_PATCHES)/wkfPatch.s
	@rm -f *.o *.bin *.elf


#------------------------------------------------------------------

build-libfat-frag:
	@cd $(SOURCES)/libfat-frag/src && make cube-release
	@cp $(SOURCES)/libfat-frag/src/libogc/lib/cube/libfat.a libfat.a
	@rm -rf $(SOURCES)/libfat-frag/src/libogc/cube_release
	@rm -rf $(SOURCES)/libfat-frag/src/libogc/lib
