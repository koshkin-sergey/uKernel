#
# Copyright (C) 2015 Sergey Koshkin <koshkin.sergey@gmail.com>
# All rights reserved
#
# File Name  :	path.mk
# Description:	Файл сборки проекта
#

# set the locations of the tools
ifndef DIR_TOOLS 
DIR_TOOLS = $(CURDIR)/../../../../../tools
endif

# set the locations of the bin and lib output directories
ifndef DIR_BIN
DIR_BIN = $(CURDIR)/bin
endif

ifndef DIR_RTOS_SRC
DIR_RTOS_SRC = $(CURDIR)/../../../../../src
endif
