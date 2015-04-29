# This makefile accepts one param: ARCH. Is mandatory.
#
#  ARCH: the following values are valid:
# 
#     ARM7TDMI
#     CORTEX-M0
#     CORTEX-M1
#     CORTEX-M3
#     CORTEX-M4
#
#  Example invocation:
#
#     $ make ARCH=CORTEX-M3
#

ifeq ("$(origin ARCH)", "command line")
  CPU := --cpu=$(ARCH)
  
ifeq ($(ARCH), $(filter CORTEX%, $(ARCH)))
  ARCH_DIR = arch/cortex_m3
  ARCH_OBJS = $(ARCH_DIR)/tn_port_cm3_armcc.o
endif

ifeq ($(ARCH), $(filter ARM7%, $(ARCH)))
  ARCH_DIR = arch/arm
endif
  
endif

ifndef ARCH_DIR
  $(error ARCH is undefined. See comments in the Makefile-single for usage notes)
endif

LIB = tnkernel.lib

# set variable to compile with ARM compiler
CC = armcc
AS = armasm
LD = armlink
AR = armar
RM = rm

INC := -I $(CURDIR)/include

DEPFLAGS = --depend=$(@:.o=.d) --depend_format=unix_escaped
CFLAGS = $(INC) $(CPU) $(DEPFLAGS) --apcs=/interwork --c99 -O2 -Otime --split_sections
ASFLAGS = $(CPU) $(DEPFLAGS)

dirs := kernel \
        $(ARCH_DIR)

FILES := $(foreach dir,$(dirs),$(wildcard $(dir)/*.c))
OBJS := $(ARCH_OBJS) $(FILES:.c=.o)
DEPS := $(OBJS:.o=.d)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

# All Target
all: $(LIB)

# Tool invocations
$(LIB): $(OBJS)
	@echo "Building target: $@"
	@$(AR) -r $@ $? 

# Other Targets
clean:
	@echo "Cleaning ..."
	$(RM) -f $(OBJS) $(DEPS) $(LIB)

.PHONY: all clean
#.SILENT:
