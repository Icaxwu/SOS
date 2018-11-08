CROSS_COMPILE	= arm-none-linux-gnueabi-
AS				= $(CROSS_COMPILE)as
LD				= $(CROSS_COMPILE)ld
CC				= $(CROSS_COMPILE)gcc
CPP				= $(CC) -E
AR				= $(CROSS_COMPILE)ar
NM				= $(CROSS_COMPILE)nm

STRIP			= $(CROSS_COMPILE)strip
OBJCOPY			= $(CROSS_COMPILE)objcopy
OBJDUMP			= $(CROSS_COMPILE)objdump

RM				= rm
export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP
export RM
TOPDIR			:= $(CURDIR)

INCLUDEDIR 		:= $(TOPDIR)/include
#WFLAGS			:= -Wall
WFLAGS			:= -w
CFLAGS 			:= -std=gnu99 $(WFLAGS) -O2 -fno-builtin -march=armv4t -mtune=arm920t -nostdlib -nostdinc -msoft-float -fsigned-char -fno-omit-frame-pointer
CFLAGS   		+= -I$(INCLUDEDIR) -I$(TOPDIR)/ucos2/SOURCE -iquote$(TOPDIR)/ucos2/PORT \
				-iquote$(TOPDIR)/uCGUI/Config \
				-iquote$(TOPDIR)/uCGUI/GUI/Core -iquote$(TOPDIR)/uCGUI/GUI/WM -iquote$(TOPDIR)/uCGUI/GUI/Widget \
				-I$(TOPDIR)/lwip/include -iquote$(TOPDIR)/lwip/include/arch \
				-iquote$(TOPDIR)/lwip/include/ipv4 \
				-I$(TOPDIR)/drivers \
				-I$(TOPDIR)/fs/Fatfs_f8a \
				-iquote$(TOPDIR)/src/helix/pub
LDFLAGS			:= -L$(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -L$(TOPDIR)/uCGUI
#LDFLAGS		+= -lucgui
LDFLAGS			+= -lgcc
LDFLAGS			+= -Tucosii.lds 

export CFLAGS LDFLAGS
export TOPDIR

TARGET := ucosii

obj-y += init/
obj-y += drivers/
obj-y += src/
obj-y += lib/
obj-y += mm/
#obj-y += ucos2/
#obj-y += uCGUI/
#obj-y += lwip/
obj-y += fs/
obj-y += sound/

.PHONY : all dis download clean distclean
all:
	@echo $(shell pwd):
	@make -s -C ./ -f $(TOPDIR)/Makefile.build
	@$(LD) -o system_temp built-in.o $(LDFLAGS)
	@gcc -o kallsyms $(TOPDIR)/scripts/kallsyms.c
	@nm -n system_temp | ./kallsyms > kallsyms.S
	@$(CC) $(CFLAGS) -c -o kallsyms.o kallsyms.S
	@echo LD system
	@$(LD) -o system built-in.o kallsyms.o $(LDFLAGS)
	@echo OBJCOPY $(TARGET).bin
	@$(OBJCOPY) -O binary -S system $(TARGET).bin

dis:system
	@echo OBJDUMP $(TARGET).dis
	@$(OBJDUMP) -D -m arm system > $(TARGET).dis

dnw:$(TARGET).bin
	dnw $(TARGET).bin

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(shell find -name "*.a")
	rm -f $(shell find -name "*.mac")
	rm -f $(TARGET) $(TARGET).dis $(TARGET).bin system system_temp kallsyms.S kallsyms.exe