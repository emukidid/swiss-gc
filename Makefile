#---------------------------------------------------------------------------------
REVISION  = git rev-list HEAD | wc -l
SVN_REVISION = swiss_r$(shell $(REVISION))

DIR     = $(DEVKITPPC)/bin
BIN2S   = $(DEVKITPRO)/tools/bin/bin2s
CC      = $(DIR)/powerpc-eabi-gcc
LD      = $(DIR)/powerpc-eabi-ld
OBJDUMP = $(DIR)/powerpc-eabi-objdump
OBJCOPY = $(DIR)/powerpc-eabi-objcopy

DIST          = dist
SOURCES       = cube
BUILDTOOLS    = buildtools
PATCHES       = $(SOURCES)/patches
AR_SOURCES    = $(SOURCES)/actionreplay

ifeq ($(OS),Windows_NT)
DOLLZ         = $(BUILDTOOLS)/dollz3.exe
DOL2GCI       = $(BUILDTOOLS)/dol2gci.exe
MKISOFS       = $(BUILDTOOLS)/mkisofs.exe
else
DOLLZ         = $(BUILDTOOLS)/dollz3
DOL2GCI       = $(BUILDTOOLS)/dol2gci
MKISOFS       = mkisofs
endif

BUILT_PATCHES = patches
GECKOSERVER   = pc/usbgecko

#------------------------------------------------------------------
.NOTPARALLEL:

# Ready to go .7z file with every type of DOL we can think of
all: clean compile-patches compile build recovery-iso build-gci build-AR build-geckoserver package

# For dev use only, avoid the unnecessary fluff
dev: clean compile-patches compile

clean:
	@echo Building on $(OS)
	@rm -rf $(DIST)
	@cd $(SOURCES)/swiss && $(MAKE) clean
	@cd $(GECKOSERVER) && $(MAKE) clean

#------------------------------------------------------------------
compile-patches:
	@cd $(PATCHES) && $(MAKE)

compile: # compile
	@cd $(SOURCES)/swiss && $(MAKE)

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
	@mkdir $(DIST)/GCI
	@mkdir $(DIST)/WODE
	@cp $(SOURCES)/swiss/swiss.dol $(DIST)/DOL/$(SVN_REVISION).dol
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/$(SVN_REVISION).dol
	@cp $(SOURCES)/swiss/swiss.elf $(DIST)/DOL/$(SVN_REVISION).elf
	@$(DOLLZ) $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/DOL/Viper/$(SVN_REVISION)-lz-viper.dol -v -m
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/Viper/$(SVN_REVISION)-lz-viper.dol
	@$(DOLLZ) $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/DOL/$(SVN_REVISION)-compressed.dol -m
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/$(SVN_REVISION)-compressed.dol
	@cp $(DIST)/DOL/$(SVN_REVISION)-compressed.dol $(DIST)/ActionReplay/swiss-lz.dol
	@cp $(DIST)/DOL/$(SVN_REVISION)-compressed.dol $(DIST)/ISO/DOL/$(SVN_REVISION)-compressed.dol
	# make ISOs and WKF firmware
	# NTSC
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-u.hdr -no-emul-boot -b $(SVN_REVISION)-compressed.dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-u)".iso $(DIST)/ISO/DOL/
	## NTSC-J
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-j.hdr -no-emul-boot -b $(SVN_REVISION)-compressed.dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-j)".iso $(DIST)/ISO/DOL/
	# PAL
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-e.hdr -no-emul-boot -b $(SVN_REVISION)-compressed.dol -o $(DIST)/ISO/$(SVN_REVISION)"(pal)".iso $(DIST)/ISO/DOL/
	# WODE
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-wode.hdr -no-emul-boot -b $(SVN_REVISION)-compressed.dol -o $(DIST)/WODE/$(SVN_REVISION)"(wode_extcfg)".iso $(DIST)/ISO/DOL/
	@cp $(BUILDTOOLS)/wkf/wkf_menu.fzn $(SVN_REVISION).fzn
	@dd if=$(DIST)/ISO/$(SVN_REVISION)"(pal)".iso of=$(SVN_REVISION).fzn conv=notrunc
	@mv $(SVN_REVISION).fzn $(DIST)/WiikeyFusion
	@cp $(BUILDTOOLS)/wkf/autoboot.fzn.fw $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn.fw
	@rm $(DIST)/ISO/DOL/$(SVN_REVISION)-compressed.dol
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
	@mv $(DIST)/GCI $(SVN_REVISION)
	@mv $(DIST)/ISO $(SVN_REVISION)
	@mv $(DIST)/WODE $(SVN_REVISION)
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

build-gci: # make GCI for memory cards
	@cp $(DIST)/DOL/$(SVN_REVISION)-compressed.dol boot.dol
	@$(DOL2GCI) boot.dol $(DIST)/GCI/boot.gci
	@rm boot.dol
	
#------------------------------------------------------------------


build-geckoserver:
	@cd $(GECKOSERVER) && $(MAKE)
	@mkdir $(DIST)/USBGeckoRemoteServer
	@mv $(GECKOSERVER)/swissserver* $(DIST)/USBGeckoRemoteServer/
	
#------------------------------------------------------------------

build-libfat-frag:
	@cd $(SOURCES)/libfat-frag/src && $(MAKE) cube-release
	@cp $(SOURCES)/libfat-frag/src/libogc/lib/cube/libfat.a libfat.a
	@rm -rf $(SOURCES)/libfat-frag/src/libogc/cube_release
	@rm -rf $(SOURCES)/libfat-frag/src/libogc/lib
