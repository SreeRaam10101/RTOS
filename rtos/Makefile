CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
CFLAGS  = -mcpu=cortex-m3 -mthumb -nostdlib -nostartfiles -ffreestanding -Wall -Wextra -O0 -g
LDFLAGS = -T linker.ld -nostdlib -nostartfiles

OBJS = boot.o pendsv.o kernel.o scheduler.o systick.o sync.o uart.o main.o

all: build/kernel.elf

build:
	mkdir -p build

build/kernel.elf: $(OBJS) linker.ld | build
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

boot.o: boot.s
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build *.o

.PHONY: all clean
