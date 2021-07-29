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
MKISOFS       = xorrisofs
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
	@cd $(PATCHES) && $(MAKE) clean
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
	@mkdir $(DIST)/WiikeyFusion
	@mkdir $(DIST)/WiikeyFusion/RecoveryISO
	@mkdir $(DIST)/ActionReplay
	@mkdir $(DIST)/GCI
	@mkdir $(DIST)/WODE
	@mkdir $(DIST)/GCLoader
	@cp $(SOURCES)/swiss/swiss.dol $(DIST)/DOL/$(SVN_REVISION).dol
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/$(SVN_REVISION).dol
	@cp $(SOURCES)/swiss/swiss.elf $(DIST)/DOL/$(SVN_REVISION).elf
	@$(DOLLZ) $(SOURCES)/swiss/swiss.dol $(DIST)/DOL/Viper/$(SVN_REVISION)-lz-viper.dol -v -m
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/Viper/$(SVN_REVISION)-lz-viper.dol
	@$(DOLLZ) $(SOURCES)/swiss/swiss.dol $(DIST)/DOL/$(SVN_REVISION)-compressed.dol -m
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/$(SVN_REVISION)-compressed.dol
	@cp $(DIST)/DOL/$(SVN_REVISION)-compressed.dol $(DIST)/ActionReplay/AUTOEXEC.DOL
	# make ISOs and WKF firmware
	# NTSC-J
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-j.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-j)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# NTSC
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-u.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-u)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# PAL
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-e.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/ISO/$(SVN_REVISION)"(pal)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# GCLoader
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-gcode.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/GCLoader/boot.iso $(DIST)/DOL/$(SVN_REVISION).dol
	# WODE
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-wode.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/WODE/$(SVN_REVISION)"(wode_extcfg)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# WKF
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-gcode.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION)-compressed.dol -o $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn $(DIST)/DOL/$(SVN_REVISION)-compressed.dol
	@truncate -s 1856K $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn
	@cp $(BUILDTOOLS)/wkf/autoboot.fzn.fw $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn.fw

#------------------------------------------------------------------

recovery-iso:
	@cp $(DIST)/ISO/$(SVN_REVISION)"(pal)".iso $(DIST)/WiikeyFusion/RecoveryISO/$(SVN_REVISION)"_Recovery".iso
	# merge bootloader and swiss
	@dd if=$(BUILDTOOLS)/wkf/recovery_bootloader.iso of=$(DIST)/WiikeyFusion/RecoveryISO/$(SVN_REVISION)"_Recovery".iso bs=32K count=1 conv=notrunc

#------------------------------------------------------------------

package:   # create distribution package
	@mkdir $(SVN_REVISION)
	@mv $(DIST)/DOL $(SVN_REVISION)
	@mv $(DIST)/GCI $(SVN_REVISION)
	@mv $(DIST)/ISO $(SVN_REVISION)
	@mv $(DIST)/WODE $(SVN_REVISION)
	@mv $(DIST)/WiikeyFusion $(SVN_REVISION)
	@mv $(DIST)/GCLoader $(SVN_REVISION)
	@mv $(DIST)/ActionReplay $(SVN_REVISION)
	@mv $(DIST)/USBGeckoRemoteServer $(SVN_REVISION)
	@find ./$(SVN_REVISION) -type f -print0 | xargs -0 sha256sum > $(SVN_REVISION).sha256
	@mv $(SVN_REVISION).sha256 $(SVN_REVISION)
	@git log -n 4 > $(SVN_REVISION)-changelog.txt
	@sed -i "s/emukidid <emukidid@gmail.com>/emu_kidid/g" $(SVN_REVISION)-changelog.txt
	@mv $(SVN_REVISION)-changelog.txt $(SVN_REVISION)
	@cp $(BUILDTOOLS)/SWISS_FILE_DESCRIPTIONS.txt $(SVN_REVISION)
	@7z a -m0=LZMA $(SVN_REVISION).7z $(SVN_REVISION)
	@tar cfJ $(SVN_REVISION).tar.xz $(SVN_REVISION)

#------------------------------------------------------------------

build-AR: # make ActionReplay
	@$(BIN2S) $(DIST)/ActionReplay/AUTOEXEC.DOL > $(AR_SOURCES)/autoexec.s
	@$(CC) -O2 -ffreestanding -c $(AR_SOURCES)/startup.s -o $(AR_SOURCES)/startup.o
	@$(CC) -O2 -ffreestanding -c $(AR_SOURCES)/autoexec.s -o $(AR_SOURCES)/autoexec.o
	@$(CC) -O2 -ffreestanding -c $(AR_SOURCES)/main.c -o $(AR_SOURCES)/main.o
	@$(LD) -o $(AR_SOURCES)/sdloader.elf $(AR_SOURCES)/startup.o $(AR_SOURCES)/main.o $(AR_SOURCES)/autoexec.o --section-start .text=0x81700000
	@$(OBJCOPY) -O binary $(AR_SOURCES)/sdloader.elf $(DIST)/ActionReplay/SDLOADER.BIN
	@rm -f $(AR_SOURCES)/*.o $(AR_SOURCES)/*.elf $(AR_SOURCES)/autoexec.s

#------------------------------------------------------------------

build-gci: # make GCI for memory cards
	@$(DOL2GCI) $(DIST)/DOL/$(SVN_REVISION)-compressed.dol $(DIST)/GCI/boot.gci boot.dol
	@cp $(BUILDTOOLS)/dol2gci* $(DIST)/GCI/

#------------------------------------------------------------------

build-geckoserver:
	@cd $(GECKOSERVER) && $(MAKE)
	@mkdir $(DIST)/USBGeckoRemoteServer
	@mv $(GECKOSERVER)/swissserver* $(DIST)/USBGeckoRemoteServer/
