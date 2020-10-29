# Simple makefile.

# Device specific info
DEVICE_FULL=stm32f103c8t6
DEVICE_SHORTER=stm32f103
DEVICE_SHORTEST=stm32f1
MDEV=-mcpu=cortex-m3 -mthumb

# Paths.  Modify as needed.
PREFIX=arm-none-eabi-
OPENCM3=/home/harold/build/libopencm3
OPENCM3_LIB=$(OPENCM3)/lib
OPENCM3_INC=$(OPENCM3)/include

# Tools
CC=$(PREFIX)gcc
LD=$(PREFIX)ld
NM=$(PREFIX)nm
OBJCOPY=$(PREFIX)objcopy
SIZE=$(PREFIX)size
STFLASH=/home/harold/build/stlink-master/build/Release/st-flash
STM32FLASH=stm32flash

DFLAGS+=-D$(shell echo $(DEVICE_FULL) |tr '[a-z]' '[A-Z]')
DFLAGS+=-D$(shell echo $(DEVICE_SHORTER) |tr '[a-z]' '[A-Z]')
DFLAGS+=-D$(shell echo $(DEVICE_SHORTEST) |tr '[a-z]' '[A-Z]')
IFLAGS+=-I$(OPENCM3_INC)
LIBS=--static -nostartfiles -Tlinkscript.ld -L$(OPENCM3_LIB) \
-lopencm3_$(DEVICE_SHORTEST) -Wl,--start-group -lc -lgcc -lnosys \
-Wl,--end-group -Wl,-Map=map -Wl,--cref -Wl,--gc-sections

CFLAGS=-O -std=c99 $(MDEV) -Wall -Wundef -Wstrict-prototypes \
-fno-common \
-ffunction-sections -fdata-sections -MD -ggdb3
CFLAGS_O2=-O2 -std=c99 $(MDEV) -Wall -Wundef -Wstrict-prototypes \
-fno-common \
-ffunction-sections -fdata-sections -MD -ggdb3
CFLAGS_NOOPT=-std=c99 $(MDEV) -Wall -Wundef -Wstrict-prototypes \
-fno-common \
-ffunction-sections -fdata-sections -MD -ggdb3

%.list:%.c
	$(CC) $(DFLAGS) $(CFLAGS) $(IFLAGS) -E $<
%.objdump:%.o
	$(PREFIX)objdump -dS $< >$@
%.o:%.c
	$(CC) $(DFLAGS) $(CFLAGS) $(IFLAGS) -c $< -o $@
%.s:%.c
	$(CC) $(DFLAGS) $(CFLAGS) $(IFLAGS) -S -fverbose-asm $< -o $@
%.dep:%.c
	$(CC) $(DFLAGS) $(IFLAGS) -MM $< |grep -v '/'
%.clean:
	rm -f $*.o $*.d $*.bin $*.elf $*.map map
%.bin:%.elf
	$(OBJCOPY) -Obinary $< $@
%.stflash:%.bin
	$(STFLASH) write $*.bin 0x8000000
PORT ?= /dev/ttyUSB0
%.stm32flash:%.bin
	$(STM32FLASH) -w $*.bin $(PORT)
%.read:
	$(STFLASH) read $@ 0x8000000 8192
%.erase:
	$(STFLASH) erase

linkscript.ld: $(OPENCM3)/ld/linker.ld.S $(OPENCM3)/scripts/genlink.py \
$(OPENCM3)/ld/devices.data
	$(CC) $(shell $(OPENCM3)/scripts/genlink.py $(OPENCM3)/ld/devices.data $(DEVICE_FULL) DEFS) -E $< |grep -v '^#' >$@

