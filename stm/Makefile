#based on Makefile from https://freeelectron.ro/bare-metal-stm32-led-blink/

ST_CLT_PREFIX = /home/dima/st/stm32cubeclt_1.20.0_2

CC = $(ST_CLT_PREFIX)/GNU-tools-for-STM32/bin/arm-none-eabi-gcc
AS = $(ST_CLT_PREFIX)/GNU-tools-for-STM32/bin/arm-none-eabi-as
LD = $(ST_CLT_PREFIX)/GNU-tools-for-STM32/bin/arm-none-eabi-ld
BIN = $(ST_CLT_PREFIX)/GNU-tools-for-STM32/bin/arm-none-eabi-objcopy
GDB = $(ST_CLT_PREFIX)/GNU-tools-for-STM32/bin/arm-none-eabi-gdb
STL = st-flash
CFLAGS = -mthumb -mcpu=cortex-m3 -g -Wl,-Map -Wl,mapfile -Wl,--cref

all: app.bin

crt.o: crt.s
	$(AS) -o crt.o crt.s

main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

app.elf: linker.ld crt.o main.o
	$(LD) -T linker.ld -o app.elf crt.o main.o

app.bin: app.elf
	$(BIN) -O binary app.elf app.bin

clean:
	rm -f *.o *.elf *.bin

flash: app.bin
	$(STL) write app.bin 0x8000000

erase:
	$(STL) erase

debug: app.elf
	$(GDB) app.elf -ex "target extended-remote localhost:61234"
