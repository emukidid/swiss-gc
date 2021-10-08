#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/base_rules

PORTLIBS	:=	$(PORTLIBS_PATH)/gamecube $(PORTLIBS_PATH)/ppc

export PATH	:=	$(PORTLIBS_PATH)/gamecube/bin:$(PORTLIBS_PATH)/ppc/bin:$(PATH)

export	LIBOGC_INC	:=	$(DEVKITPRO)/libogc2/include
export	LIBOGC_LIB	:=	$(DEVKITPRO)/libogc2/lib/cube


MACHDEP =  -DGEKKO -mvgc -mcpu=750 -meabi -mhard-float

#---------------------------------------------------------------------------------
%.dol: %.elf
	$(SILENTMSG) output ... $(notdir $@)
	$(SILENTCMD)elf2dol $< $@

#---------------------------------------------------------------------------------
%.tpl : %.scf
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)gxtexconv -s $< -d $(DEPSDIR)/$*.d -o $@

#---------------------------------------------------------------------------------
%.elf:
	$(SILENTMSG) linking ... $(notdir $@)
	$(SILENTCMD)$(LD)  $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@

#---------------------------------------------------------------------------------
%.o: %.c
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	swiss
BUILD		:=	build
SOURCES		:=  source \
              source/aram \
			  source/gui \
			  source/config \
              source/devices \
              source/devices/system \
              source/devices/dvd \
              source/devices/fat \
			  source/devices/fsp \
			  source/devices/ftp \
			  source/devices/gcloader \
              source/devices/memcard \
              source/devices/qoob \
			  source/devices/smb \
			  source/devices/usbgecko \
              source/devices/wode \
			  source/devices/wiikeyfusion \
              source/cheats \
              source/patches \
			  source/patches/dvd \
			  source/patches/gx \
			  source/patches/mtx \
			  source/patches/vi \
			  source/psoarchive \
              source/images \
			  source/images/buttons \
			  source/fatfs
DATA		:=	data  
TEXTURES	:=	source/images \
              source/images/buttons
INCLUDES	:= include $(SOURCES)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS		= -g -O3 -Wall -Wno-format-truncation -fno-strict-aliasing $(MACHDEP) $(INCLUDE) -D'GITREVISION="$(shell git rev-parse --short HEAD)"' -D'GITVERSION="$(shell git rev-list HEAD | wc -l)"'
CXXFLAGS	= $(CFLAGS)

LDFLAGS		= $(CFLAGS) -Wl,-Map,$(notdir $@).map -Wl,--section-start,.init=0x80003100 -T../../linker-script/vgcogc.ld -T ../../reservedarea.ld

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	-lbba -lmad -lasnd -logc -ltinysmb -ldb -lz

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(PORTLIBS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(TEXTURES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
SCFFILES	:=	$(foreach dir,$(TEXTURES),$(notdir $(wildcard $(dir)/*.scf)))
TPLFILES	:=	$(SCFFILES:.scf=.tpl)

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) $(addsuffix .o,$(TPLFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES))) $(addsuffix .h,$(subst .,_,$(TPLFILES)))

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	-L$(LIBOGC_LIB) $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@echo Current Directory $(CURDIR)
	@echo GIT Revision $(shell git rev-parse --short HEAD)
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile RUNDIR=$(CURDIR)

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol

#---------------------------------------------------------------------------------
run:
	wiiload $(TARGET).dol

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the .tpl extension
#---------------------------------------------------------------------------------
%.tpl.o	%_tpl.h :	%.tpl
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
