
ifeq ($(findstring CYGWIN, $(OS)), CYGWIN)
	ELF2BIN = $(AMEBAZ2_TOOLDIR)/elf2bin.exe
	CHKSUM = $(AMEBAZ2_TOOLDIR)/checksum.exe
	TOOLCHAIN_FILENAME_CURR = asdk-6.5.0-cygwin-newlib-build-3215-i686.tar.bz2
	TOOLCHAIN_FILENAME_PREV = asdk-6.4.1-cygwin-newlib-build-2778-i686.tar.bz2
	TOOLCHAIN_BITS = 32
endif

ifeq ($(findstring MINGW32, $(OS)), MINGW32)
	ELF2BIN = $(AMEBAZ2_TOOLDIR)/elf2bin.exe
	CHKSUM = $(AMEBAZ2_TOOLDIR)/checksum.exe
	TOOLCHAIN_FILENAME_CURR = asdk64-6.4.1-mingw32-newlib-build-3026.zip
	TOOLCHAIN_FILENAME_PREV = asdk-5.5.1-mingw32-newlib-build-2754.zip
	TOOLCHAIN_BITS = 32
endif

ifeq ($(findstring Linux, $(OS)), Linux)
	ELF2BIN = $(AMEBAZ2_GCCTOOLDIR)/elf2bin.linux	
	CHKSUM = $(AMEBAZ2_GCCTOOLDIR)/checksum.linux
	TOOLCHAIN_FILENAME_CURR = asdk-6.5.0-linux-newlib-build-3215-x86_64.tar.bz2
	TOOLCHAIN_FILENAME_PREV = asdk-6.4.1-linux-newlib-build-3026-x86_64.tar.bz2
	TOOLCHAIN_BITS = 64
endif

ifeq ($(findstring Darwin, $(OS)), Darwin)
	ELF2BIN = $(AMEBAZ2_GCCTOOLDIR)/elf2bin.darwin
	CHKSUM = $(AMEBAZ2_GCCTOOLDIR)/checksum.darwin
	TOOLCHAIN_FILENAME_CURR = asdk-6.5.0-darwin-newlib-build-999+-10.13.tar.bz2
	TOOLCHAIN_FILENAME_PREV = asdk-6.5.0-darwin-newlib-build-999+-10.13.tar.bz2
	TOOLCHAIN_BITS = 64
endif

# Choose toolchain 
ifneq (,$(wildcard ../../../tools/arm-none-eabi-gcc/$(TOOLCHAIN_FILENAME_CURR)))
     TOOLCHAIN_FILENAME = $(TOOLCHAIN_FILENAME_CURR)
else ifneq (,$(wildcard ../../../tools/arm-none-eabi-gcc/$(TOOLCHAIN_FILENAME_PREV)))
     TOOLCHAIN_FILENAME = $(TOOLCHAIN_FILENAME_PREV)
endif 

word-dash = $(word $2,$(subst -, ,$1))

TOOLCHAIN_VER = $(call word-dash,$(TOOLCHAIN_FILENAME), 2)
ASDK_PLATFORM = $(call word-dash,$(TOOLCHAIN_FILENAME), 3)

ARM_GCC_TOOLCHAIN = ../../../tools/arm-none-eabi-gcc/asdk/$(ASDK_PLATFORM)/newlib/bin

# get extension and potential extracting folder, filename or asdk-(ver) 
EXTRACT_FOLDER1 := asdk-$(TOOLCHAIN_VER)
EXT1 := $(suffix $(TOOLCHAIN_FILENAME))
EXTRACT_FOLDER2 := $(basename $(TOOLCHAIN_FILENAME))

ifneq ($(EXT1), .zip)
	EXT := $(suffix $(EXTRACT_FOLDER2))
	EXTRACT_FOLDER2 := $(basename $(EXTRACT_FOLDER2))
else
	EXT := $(EXT1)
endif

##extract = tar -jxf $1 -C $2
##extract = unzip -q -u $1

LBITS := $(shell getconf LONG_BIT)

.PHONY: toolchain
toolchain:
	@echo Toolchain $(TOOLCHAIN_FILENAME) extracting...
ifneq ("$(LBITS)", "$(TOOLCHAIN_BITS)")
	@echo ONLY $(TOOLCHAIN_BITS)-BIT CYGWIN IS SUPPORTED!
	@exit -1
endif
	@if [ ! -d ../../../tools/arm-none-eabi-gcc/asdk ]; then mkdir ../../../tools/arm-none-eabi-gcc/asdk; fi
	@if [ ! -d ../../../tools/arm-none-eabi-gcc/asdk/$(ASDK_PLATFORM) ] ; then \
		if [ -f ../../../tools/arm-none-eabi-gcc/$(TOOLCHAIN_FILENAME) ] ; then \
			if [ "$(EXT)" == ".tar" ] ; then \
				tar -jxf ../../../tools/arm-none-eabi-gcc/$(TOOLCHAIN_FILENAME) -C ../../../tools/arm-none-eabi-gcc/ ; \
				if [ -d ../../../tools/arm-none-eabi-gcc/$(EXTRACT_FOLDER1) ]; then \
					mv ../../../tools/arm-none-eabi-gcc/$(EXTRACT_FOLDER1)/$(ASDK_PLATFORM) ../../../tools/arm-none-eabi-gcc/asdk/$(ASDK_PLATFORM) ; \
					rmdir ../../../tools/arm-none-eabi-gcc/$(EXTRACT_FOLDER1); \
				elif [ -d ../../../tools/arm-none-eabi-gcc/$(EXTRACT_FOLDER2) ]; then \
					mv ../../../tools/arm-none-eabi-gcc/$(EXTRACT_FOLDER2)/$(ASDK_PLATFORM) ../../../tools/arm-none-eabi-gcc/asdk/$(ASDK_PLATFORM) ; \
					rmdir ../../../tools/arm-none-eabi-gcc/$(EXTRACT_FOLDER2); \
				fi; \
			elif [ "$(EXT)" == ".zip" ] ; then \
				unzip -q -u ../../../tools/arm-none-eabi-gcc/$(TOOLCHAIN_FILENAME); \
				if [ -d $(EXTRACT_FOLDER1) ]; then \
					mv $(EXTRACT_FOLDER1)/$(ASDK_PLATFORM) ../../../tools/arm-none-eabi-gcc/asdk/$(ASDK_PLATFORM) ; \
					rmdir $(EXTRACT_FOLDER1); \
				elif [ -d $(EXTRACT_FOLDER2) ]; then \
					mv $(EXTRACT_FOLDER2)/$(ASDK_PLATFORM) ../../../tools/arm-none-eabi-gcc/asdk/$(ASDK_PLATFORM) ; \
					rmdir $(EXTRACT_FOLDER2); \
				fi; \
			fi; \
		else \
			echo Not exist $(TOOLCHAIN_FILENAME); \
			exit -1;\
		fi;\
	fi	
	

	@echo Toolchain extracting done!
