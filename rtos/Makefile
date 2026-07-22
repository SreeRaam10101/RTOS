CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc

SCHED    ?= RMS
WORKLOAD ?= 1

SCHED_FLAG =
ifeq ($(SCHED),EDF)
SCHED_FLAG = -DSCHED_EDF=1
endif

CFLAGS  = -mcpu=cortex-m3 -mthumb -nostdlib -nostartfiles -ffreestanding -Wall -Wextra -O0 -g $(SCHED_FLAG) -DWORKLOAD=$(WORKLOAD)
LDFLAGS = -T linker.ld -nostdlib -nostartfiles

OBJS = boot.o pendsv.o kernel.o scheduler.o systick.o sync.o rtos.o uart.o queue.o adc.o gpio.o main.o
OBJS_NO_INHERIT = boot.o pendsv.o kernel.o scheduler.o systick.o sync_no_inherit.o rtos.o uart.o queue.o adc.o gpio.o main.o

all: build/kernel.elf build/kernel_no_inherit.elf

build:
	mkdir -p build

build/kernel.elf: $(OBJS) linker.ld | build
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

build/kernel_no_inherit.elf: $(OBJS_NO_INHERIT) linker.ld | build
	$(CC) $(LDFLAGS) -o $@ $(OBJS_NO_INHERIT)

sync_no_inherit.o: sync.c
	$(CC) $(CFLAGS) -DPRIORITY_INHERITANCE_ENABLED=0 -c -o $@ $<

boot.o: boot.s
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build *.o

.PHONY: all clean
