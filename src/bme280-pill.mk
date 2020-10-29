# Simple makefile for bme280-pill.
include include.mk

all:linkscript.ld bme280-pill.elf bme280-pill.bin

clean:
	rm -f bme280-pill.bin *.o *.d *.elf *.map map

bme280-pill.elf:bme280-pill.o cal32.o tx.o fmt.o uart_setup.o \
msn.o sensors.o command.o vdd_mv.o mcu.o bosch.o wi2c.o wgpio.o ymodem.o
	$(CC) $(MDEV) $^ $(LIBS) -o $@

test-stm32flash:
	echo $(STM32FLASH)
