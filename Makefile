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
PACKER        = $(SOURCES)/packer

ifeq ($(OS),Windows_NT)
DOLLZ         = $(BUILDTOOLS)/dollz3.exe
DOL2GCI       = $(BUILDTOOLS)/dol2gci.exe
else
DOLLZ         = $(BUILDTOOLS)/dollz3
DOL2GCI       = $(BUILDTOOLS)/dol2gci
endif
DOL2IPL       = $(BUILDTOOLS)/dol2ipl.py

ifneq ($(shell which mkisofs 2>/dev/null),)
MKISOFS       = mkisofs
else
MKISOFS       = xorrisofs
endif
ifneq ($(shell which dkp-pacman 2>/dev/null),)
PACMAN        = dkp-pacman
else
PACMAN        = pacman
endif

GECKOSERVER   = pc/usbgecko
WIIBOOTER     = wii/booter

#------------------------------------------------------------------
.NOTPARALLEL:

# Ready to go .7z file with every type of DOL we can think of
all: clean compile-patches compile compile-packer build recovery-iso build-AR build-gci build-ipl build-wii build-geckoserver package

# For dev use only, avoid the unnecessary fluff
dev: clean compile-patches compile

clean:
	@echo Building on $(OS)
	@rm -rf $(DIST)
	@cd $(PATCHES) && $(MAKE) clean
	@cd $(SOURCES)/swiss && $(MAKE) clean
	@cd $(PACKER) && $(MAKE) clean
	@cd $(GECKOSERVER) && $(MAKE) clean

#------------------------------------------------------------------
compile-patches:
	@cd $(PATCHES) && $(MAKE)

compile: # compile
	@$(PACMAN) -Q gamecube-tools-git libogc2-git libogc2-libdvm-git ppc-libdeflate ppc-libmad ppc-libpsoarchive ppc-libxxhash ppc-zlib-ng-compat
	@cd $(SOURCES)/swiss && $(MAKE)

compile-packer:
	@cd $(PACKER) && $(MAKE)

#------------------------------------------------------------------

build:
	# create initial DIR structure and various DOLs 
	@mkdir $(DIST)
	@mkdir $(DIST)/DOL
	@mkdir $(DIST)/DOL/Legacy
	@mkdir $(DIST)/FlippyDrive
	@mkdir $(DIST)/GCLoader
	@mkdir $(DIST)/ISO
	@mkdir $(DIST)/WiikeyFusion
	@mkdir $(DIST)/WODE
	@cp $(PACKER)/swiss.dol $(DIST)/DOL/$(SVN_REVISION).dol
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/$(SVN_REVISION).dol
	@cp $(SOURCES)/swiss/swiss.elf $(DIST)/DOL/$(SVN_REVISION).elf
	@$(DOLLZ) $(SOURCES)/swiss/swiss.dol $(DIST)/DOL/Legacy/$(SVN_REVISION)-lz.dol -m
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/Legacy/$(SVN_REVISION)-lz.dol
	@$(DOLLZ) $(SOURCES)/swiss/swiss.dol $(DIST)/DOL/Legacy/$(SVN_REVISION)-lz-viper.dol -v -m
	@echo -n $(shell git rev-parse --short HEAD) >> $(DIST)/DOL/Legacy/$(SVN_REVISION)-lz-viper.dol
	@cp $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/FlippyDrive/boot.dol
	# make ISOs and WKF firmware
	# GCLoader
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-gcode.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/GCLoader/boot.iso $(DIST)/DOL/$(SVN_REVISION).dol
	# NTSC-J
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-j.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-j)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# NTSC
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-u.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/ISO/$(SVN_REVISION)"(ntsc-u)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# PAL
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-e.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/ISO/$(SVN_REVISION)"(pal)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# WODE
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-wode.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/WODE/$(SVN_REVISION)"(wode_extcfg)".iso $(DIST)/DOL/$(SVN_REVISION).dol
	# WKF
	@$(MKISOFS) -R -J -G $(BUILDTOOLS)/iso/eltorito-gcode.hdr -no-emul-boot -eltorito-platform PPC -b $(SVN_REVISION).dol -o $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn $(DIST)/DOL/$(SVN_REVISION).dol
	@truncate -s 1856K $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn
	@cp $(BUILDTOOLS)/wkf/autoboot.fzn.fw $(DIST)/WiikeyFusion/$(SVN_REVISION).fzn.fw

#------------------------------------------------------------------

recovery-iso:
	@mkdir $(DIST)/WiikeyFusion/RecoveryISO
	@cp $(DIST)/ISO/$(SVN_REVISION)"(pal)".iso $(DIST)/WiikeyFusion/RecoveryISO/$(SVN_REVISION)"_Recovery".iso
	# merge bootloader and swiss
	@dd if=$(BUILDTOOLS)/wkf/recovery_bootloader.iso of=$(DIST)/WiikeyFusion/RecoveryISO/$(SVN_REVISION)"_Recovery".iso bs=32K count=1 conv=notrunc

#------------------------------------------------------------------

package:   # create distribution package
	@mkdir $(SVN_REVISION)
	@mv $(DIST)/ActionReplay $(SVN_REVISION)
	@mv $(DIST)/Apploader $(SVN_REVISION)
	@mv $(DIST)/DOL $(SVN_REVISION)
	@mv $(DIST)/FlippyDrive $(SVN_REVISION)
	@mv $(DIST)/GCLoader $(SVN_REVISION)
	@mv $(DIST)/ISO $(SVN_REVISION)
	@mv $(DIST)/MemoryCard $(SVN_REVISION)
	@mv $(DIST)/USBGeckoRemoteServer $(SVN_REVISION)
	@mv $(DIST)/Wii $(SVN_REVISION)
	@mv $(DIST)/WiikeyFusion $(SVN_REVISION)
	@mv $(DIST)/WODE $(SVN_REVISION)
	@mkdir $(SVN_REVISION)/PicoBoot
	@cp $(DIST)/PicoBoot/$(SVN_REVISION).uf2 $(SVN_REVISION)/PicoBoot/$(SVN_REVISION).uf2
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
	@mkdir $(DIST)/ActionReplay
	@cp $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/ActionReplay/AUTOEXEC.DOL
	@cp $(PACKER)/SDLOADER.BIN $(DIST)/ActionReplay/

#------------------------------------------------------------------

build-gci: # make GCI for memory cards
	@mkdir $(DIST)/MemoryCard
	@mkdir $(DIST)/MemoryCard/dol2gci
	@$(DOL2GCI) $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/MemoryCard/boot.gci boot.dol
	@$(DOL2GCI) $(DIST)/DOL/$(SVN_REVISION).dol $(DIST)/MemoryCard/xeno.gci xeno.dol
	@cp $(BUILDTOOLS)/dol2gci* $(DIST)/MemoryCard/dol2gci/

#------------------------------------------------------------------

build-geckoserver:
	@cd $(GECKOSERVER) && $(MAKE)
	@mkdir $(DIST)/USBGeckoRemoteServer
	@cp $(GECKOSERVER)/swissserver* $(DIST)/USBGeckoRemoteServer/

#------------------------------------------------------------------

build-ipl:
	@mkdir $(DIST)/Apploader
	@mkdir $(DIST)/Apploader/swiss
	@mkdir $(DIST)/Apploader/swiss/patches
	@mkdir $(DIST)/PicoBoot
	@$(DOL2IPL) $(DIST)/Apploader/swiss/patches/apploader.img $(PACKER)/reboot.dol *$(SVN_REVISION).dol
	@$(DOL2IPL) $(DIST)/PicoBoot/$(SVN_REVISION)_pico.uf2 $(PACKER)/reboot.dol rp2040
	@$(DOL2IPL) $(DIST)/PicoBoot/$(SVN_REVISION)_pico2.uf2 $(PACKER)/reboot.dol rp2350
	@cat $(DIST)/PicoBoot/$(SVN_REVISION)_pico.uf2 $(DIST)/PicoBoot/$(SVN_REVISION)_pico2.uf2 > $(DIST)/PicoBoot/$(SVN_REVISION).uf2

#------------------------------------------------------------------

build-wii:
	@cd $(WIIBOOTER) && $(MAKE)
	@$(DOL2IPL) $(WIIBOOTER)/boot.dol $(PACKER)/reboot.dol
	@mkdir $(DIST)/Wii
	@mkdir $(DIST)/Wii/apps
	@mkdir $(DIST)/Wii/apps/swiss-gc
	@cp $(WIIBOOTER)/boot.dol $(DIST)/Wii/apps/swiss-gc/
