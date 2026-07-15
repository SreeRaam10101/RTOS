# RTOS — From-Scratch Preemptive RTOS on ARM Cortex-M3

![CI](https://github.com/SreeRaam10101/RTOS/actions/workflows/ci.yml/badge.svg)

A preemptive real-time operating system kernel built from scratch in C and ARM Thumb-2
assembly, running on an emulated ARM Cortex-M3 (QEMU `mps2-an385`) — no RTOS library or HAL
as a base.

## What's in here

- **Context switching** — hand-written `PendSV_Handler`, synthetic exception-frame
  construction so a never-yet-run task looks identical, to the restore path, to one resuming
  mid-execution (`rtos/pendsv.s`, `rtos/kernel.c`).
- **Priority-array scheduler** with SysTick-driven time-sliced preemption (`rtos/scheduler.c`,
  `rtos/systick.c`).
- **Blocking primitives** — `delay()`, semaphore, mutex — built on a sorted wake-tick list
  with PRIMASK-protected critical sections (`rtos/sync.c`).
- **Priority inheritance** — the classic 3-task priority-inversion scenario, constructed and
  then fixed, compiled two ways from one source tree via a single flag so the comparison is a
  controlled experiment (`rtos/sync.c`, `rtos/main.c`).
- **Real-time layer** — periodic tasks, a Rate-Monotonic Scheduling (RMS) schedulability check
  (Liu & Layland bound, integer-only arithmetic), and a runtime deadline-miss counter that
  catches violations the static check doesn't (`rtos/rtos.c`).

## Building and testing

```bash
cd rtos
make clean && make
cd ..
./tests/m5_test.sh   # the latest milestone's test -- see tests/README.md for why
```

Requires `arm-none-eabi-gcc`/`gdb` and `qemu-system-arm` (see CI workflow for exact packages).

## Docs

- `docs/superpowers/specs/` — the approved architecture spec
- `docs/superpowers/plans/` — one implementation plan per milestone
- `rtos-whiteboard.md` — concept notes (real-time fundamentals, schedulability math)
- `CLAUDE.md` — full project history/decisions log
