# This makefile accepts two params: TN_ARCH and TN_COMPILER. Both are mandatory.
#
#  TN_ARCH: the following values are valid:
# 
#     arm7
#     cortex_m0
#     cortex_m0plus
#     cortex_m1
#     cortex_m3
#     cortex_m4
#     cortex_m4f
#
#  TN_COMPILER: the following values are valid:
#
#     armcc
#     armclang
#
#  Example invocation:
#
#     $ make TN_ARCH=cortex_m3 TN_COMPILER=armcc
#

ERR_MSG_STD = See comments in the Makefile

ifneq ($(MAKECMDGOALS),clean)
  ifeq ($(TN_ARCH),)
    $(error TN_ARCH is undefined. $(ERR_MSG_STD))
  endif

  ifeq ($(TN_COMPILER),)
     $(error TN_COMPILER is undefined. $(ERR_MSG_STD))
  endif
  
endif

# set the locations of the bin and lib output directories
ifndef BIN_FOLDER
BIN_FOLDER = ./bin/$(TN_ARCH)/$(TN_COMPILER)
endif

ifndef SRC
SRC = ../../src
endif

ifndef OBJ_FOLDER
OBJ_FOLDER = ./obj/$(TN_ARCH)/$(TN_COMPILER)
endif

TOOLS_FOLDER = ../../tools
MKDIR = $(TOOLS_FOLDER)/mkdir
RM = $(TOOLS_FOLDER)/rm

#---------------------------------------------------------------------------
# Cortex-M series
#---------------------------------------------------------------------------

ifeq ($(TN_ARCH), $(filter $(TN_ARCH), cortex_m0 cortex_m0plus cortex_m1 cortex_m3 cortex_m4 cortex_m4f))
  
  ifeq ($(TN_COMPILER), armcc)
  
    ifeq ($(TN_ARCH), cortex_m0)
      CPU := --cpu=cortex-m0
      ARCH_DIR = $(SRC)/arch/cortex_m0
      SFILES = $(ARCH_DIR)/tn_port_cm0_armcc.s
    endif

    ifeq ($(TN_ARCH), cortex_m3)
      CPU := --cpu=cortex-m3
      ARCH_DIR = $(SRC)/arch/cortex_m3
      SFILES = $(ARCH_DIR)/tn_port_cm3_armcc.s
    endif

    ifeq ($(TN_ARCH), cortex_m4)
      CPU := --cpu=cortex-m4
      ARCH_DIR = $(SRC)/arch/cortex_m3
      SFILES = $(ARCH_DIR)/tn_port_cm3_armcc.s
    endif

    CC = armcc
    AS = armasm
    LD = armlink
    AR = armar

    DEPFLAGS = --depend=$(@:.o=.d) --depend_format=unix_escaped
    CFLAGS = $(INC) $(CPU) $(DEPFLAGS) --apcs=/interwork --c99 -O2 -Otime --split_sections
    ASFLAGS = $(CPU) $(DEPFLAGS)
    
  endif
  
endif

#---------------------------------------------------------------------------
# ARM7
#---------------------------------------------------------------------------

ifeq ($(TN_ARCH), arm7)

  ifeq ($(TN_COMPILER), armcc)
    CPU := --cpu=arm7tdmi
    ARCH_DIR = $(SRC)/arch/arm

    CC = armcc
    AS = armasm
    LD = armlink
    AR = armar

    DEPFLAGS = --depend=$(@:.o=.d) --depend_format=unix_escaped
    CFLAGS = $(INC) $(CPU) $(DEPFLAGS) --apcs=/interwork --c99 -O2 -Otime --split_sections
    ASFLAGS = $(CPU) $(DEPFLAGS)

  endif

endif

BINARY = $(BIN_FOLDER)/tnkernel.lib

INC := -I $(SRC)/include

DIRS := $(SRC)/kernel \
        $(ARCH_DIR)

CFILES := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))
OBJS := $(patsubst %.c,$(OBJ_FOLDER)/%.o, $(notdir $(CFILES))) $(patsubst %.s,$(OBJ_FOLDER)/%.o,$(notdir $(SFILES)))
DEPS := $(OBJS:.o=.d)

VPATH = $(DIRS)

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEPS)
endif

# All Target
all: $(OBJ_FOLDER) $(BIN_FOLDER) $(BINARY)

# Other Targets
clean:
	@echo Cleaning...
	@$(RM) -drf $(BIN_FOLDER) $(OBJ_FOLDER)
	@echo Done

.PHONY: all clean

$(BIN_FOLDER):
	@$(MKDIR) -p $@

$(OBJ_FOLDER):
	@$(MKDIR) -p $@

$(BINARY): $(OBJS)
	@$(AR) -r $@ $?
	@echo Done 

$(OBJ_FOLDER)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_FOLDER)/%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<
