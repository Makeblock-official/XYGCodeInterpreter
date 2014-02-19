

#Name of the program and name of the output file 
PROJECT_PATH := $(shell pwd)
PROJECT_NAME := $(notdir $(PROJECT_PATH))
OUTPUT := $(PROJECT_NAME)

#Name of the target machine and name of the output dir
CC ?= gcc
SYSTEM := $(shell uname -s 2>/dev/null)
ifeq ($(SYSTEM),)
	SYSTEM := bin
endif
MACHINE := $(shell $(CC) -dumpmachine)
ifeq ($(MACHINE),)
	MACHINE := $(SYSTEM)
endif
TARGET := $(MACHINE)/$(OUTPUT)
COMPILERS := gcc i686-w64-mingw32-gcc

#Compilers, utils
STRIP := $(shell $(CC) -print-prog-name=strip)
ifeq ($(STRIP),)
	STRIP := strip
endif
LD := $(shell $(CC) -print-prog-name=ld)
ifeq ($(LD),)
	LD := ld
endif


#Sources
HEADERS := $(shell ls *.h 2>/dev/null)
SOURCES := $(shell ls *.c 2>/dev/null)
OBJECTS := $(patsubst %.c,$(MACHINE)/%.o,$(SOURCES))

#Compiler flags
INCLUDES := 
CFLAGS := -Wall -O3
LDFLAGS := -L../lib


#Post-init check
ifneq (,$(findstring qnx,$(MACHINE)))
	#Compiling for QNX target
else
	ifneq (,$(findstring mingw,$(MACHINE)))
		#Compiling for win32 target
		TARGET := $(TARGET).exe
	else
		#Compiling for other (Linux) target
	endif
endif

ifeq ($(SYSTEM),QNX)
	#Compiling under QNX host
	COMPILERS := gcc
else
	#Compiling under other (Linux) host
endif

###########
# Targets #
###########

all:
	rm -f $(OUTPUT)
	for i in $(COMPILERS); do $(MAKE) CC=$$i target; done
	ln -s $(TARGET) $(OUTPUT)

clean:
	for i in $(COMPILERS); do $(MAKE) CC=$$i cleantarget; done
	rm -f $(OUTPUT)


##################
# primary target #
##################

target: echoatts $(MACHINE) $(TARGET)
# strip does not work properly for targets other than x86 because QNX gnu copliler does not return correct path to strip util for these targets
#	$(STRIP) $(TARGET)

echoatts:
	@echo ---
	@echo Compiling for TARGET=$(MACHINE) under HOST=$(SYSTEM), gcc=$(CC), strip=$(STRIP), ld=$(LD)
	@echo ---

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	$(STRIP) $(TARGET)

$(MACHINE)/%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(MACHINE):
	mkdir -p $(MACHINE)

################
# misc targets #
################

cleantarget:
	rm -Rf $(MACHINE)



