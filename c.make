PROJECT := test
DEFS    :=
INCS    := examples
SRCS    := examples/main.c
LIBS    :=
SCRIPT  :=
COMMON  := common

#----------------------------------------------------------#
include $(COMMON)/demos/make/stm32f4discovery/makefile.gnucc
#----------------------------------------------------------#

include $(COMMON)/cmsis/makefile
include $(COMMON)/device/nosys/makefile
include $(COMMON)/startup/makefile
include $(COMMON)/demos/makefile

#----------------------------------------------------------#
include $(COMMON)/make/makefile
#----------------------------------------------------------#