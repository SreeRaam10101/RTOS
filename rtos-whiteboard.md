# RTOS Whiteboard — Learning Notes

> Goal: build solid RTOS intuition on **QEMU (ARM Cortex-M)** before any coding.
> Audience: I already built an xv6-x64 scheduler (MLFQ+SJF + agent), signals, mmap, VM, FS.
> Style: brief. Each concept mapped to what I already know from xv6.
> Learning doc: concepts §0–§10, milestone arc §6b.

---

## 0. The one-line difference

**xv6 = fastest on average. RTOS = never late, worst-case.**
Same machinery (timer ticks, context switch, ready queue), opposite success metric: *determinism*, not throughput.

---

## 1. What "real-time" actually means

- **Hard real-time**: a missed deadline = system failure (airbag, motor control).
- **Soft real-time**: a missed deadline = degraded quality (video frame drop).
- Key metric is not speed but **predictability**: bounded, worst-case latency.
- We measure in **tick counts**, not nanoseconds — so QEMU (not wall-clock-exact) is fine for learning the *logic*.

---

## 2. Cortex-M vs x86-64 (why this is EASIER than xv6)

| Thing | xv6 (x86-64) | Cortex-M |
|-------|--------------|----------|
| MMU / page tables | Yes (painful) | **None** — flat memory |
| Interrupt entry | You save trapframe by hand | **HW auto-stacks** 8 registers |
| Interrupt controller | APIC | **NVIC** (clean, priority-based) |
| Context switch | `swtch` in asm | `PendSV` handler (asm, ~15 lines) |
| Privilege | ring 0/3 | Thread mode / Handler mode |

Takeaway: I spend energy on **scheduling logic**, not architecture arcana.

---

## 3. The RTOS canon — 3 ideas I haven't done yet

### (a) Priority-based preemptive scheduling
**Invariant:** the highest-priority *ready* task always runs. Period.
- Contrast xv6 MLFQ: priorities *decay* for fairness. RTOS: priorities are **fixed** and respected absolutely.
- Two ways to assign them:
  - **RMS (Rate-Monotonic)**: shorter period → higher priority. Static, simple, provable.
  - **EDF (Earliest-Deadline-First)**: nearest deadline → runs now. Dynamic, higher CPU utilization, harder to implement.

### (b) Priority inversion + inheritance  ← the "wow" chapter
```
Low task L holds a mutex.  High task H needs it → H blocks.
Medium task M (needs no mutex) preempts L → L can't finish → H waits forever-ish.
This froze the Mars Pathfinder rover in 1997.
```
**Fix = priority inheritance**: while L holds the mutex H wants, L temporarily runs at H's priority. M can't jump the line.

### (c) Bounded interrupt latency
- ISRs must be short and time-bounded. Long work is deferred to a task ("bottom half").
- Ties to my xv6 trap.c experience — but now the *timing* is the deliverable.

---

## 4. Schedulability — can this task set even fit?

The RTOS question xv6 never asked: *given these periods, is meeting all deadlines guaranteed?*
- **RMS bound (Liu & Layland):** N tasks are schedulable if total CPU utilization
  `U = Σ (Cᵢ / Tᵢ) ≤ N(2^(1/N) − 1)`  (→ ~0.69 as N grows).
  - Cᵢ = worst-case compute time, Tᵢ = period.
- **EDF bound:** schedulable iff `U ≤ 1.0` (uses the CPU better than RMS).
- This is a *provable* property — the kind of guarantee that makes RTOS "real-time."
- **Caveat:** the bound above assumes *implicit deadlines* (Dᵢ = Tᵢ). If a task's deadline is shorter than its period (constrained deadline, Dᵢ < Tᵢ, as in the M5 demo), the bound only proves "schedulable if deadlines equaled periods" — the real, shorter deadlines need the runtime deadline-miss counter to actually be checked.

---

## 5. QEMU setup (the workflow, not the code yet)

- Emulated board: `qemu-system-arm -M mps2-an385` (Cortex-M3) or `lm3s6965evb`.
- Toolchain: `arm-none-eabi-gcc` + `gdb`.
- Loop is identical to my xv6 flow: **build → `qemu … -S -gdb tcp::1234` → break/inspect → iterate.**
- No flashing, no USB, no "board or bug?" ambiguity.

---

## 6. Proposed project arc (build later, in layers)

Each layer maps to something I already did in xv6:

1. **Boot + context switch** — "two tasks alternate." (xv6 `swtch` analog, via PendSV)
2. **Priority preemptive scheduler** — highest-ready-priority wins. (my scheduler loop, new invariant)
3. **Blocking primitives** — `delay(ticks)`, semaphore, mutex. (xv6 sleep/wakeup analog)
4. **Priority inheritance** — build the inversion, then fix it. ← best story
5. **(Stretch) RMS or EDF + deadline-miss harness** — reuse my xv6 eval/benchmark instinct.
6. **(Dream stretch) adaptive agent** — the RL bandit I never finished, now tuning task periods against the RMS bound.

---

## 6b. Project direction — "grow-a-kernel" (DECIDED: from-scratch, option 2)

Build in milestones. Each one **boots in QEMU and demos something** — never a broken half-kernel.
Each milestone = a git tag + a short note here. This is the top-to-bottom learning spine.

```
M0  Bare-metal boot      → one task prints forever        (toolchain + linker + QEMU work)
M1  Context switch        → 2 tasks alternate via PendSV    (the heart of any RTOS = xv6 swtch)
M2  Preemptive scheduler  → SysTick tick + priority ready-queue, highest-ready runs
M3  Blocking primitives   → delay(ticks), semaphore, mutex  (tasks sleep/wake, not spin)
M4  Priority inheritance  → build the inversion, then fix it  ← the "wow" chapter
M5  Real-time layer       → periodic tasks + deadline-miss counter (RMS capstone)
M6  (stretch) adaptive    → revive the xv6 RL-bandit to tune periods; or add EDF + compare
```

Notes:
- **M1 is the whole game.** Once two tasks swap via PendSV, the rest is bookkeeping around that switch. Cortex-M HW stacks half the registers for me → simpler than xv6.
- **M4 is the teaching payoff.** Inversion is invisible until constructed; inheritance fixes it in a few lines. Best demo + best writeup material.
- **Capstone = RMS** (working assumption): fixed-priority, hand-provable bound. EDF deferred to M6 as an optional comparison (reuses the M5 deadline-miss harness).

## 7. Open questions for me to decide (with Claude)

- [x] ~~Full from-scratch kernel vs existing base?~~ → **from-scratch kernel** (option 2, grow-a-kernel, §6b)
- [~] RMS vs EDF? → **RMS capstone** (working assumption); EDF deferred to M6 comparison. Confirm.
- [ ] How to *measure* determinism in QEMU (tick-based deadline-miss counter)? — design in M5.
- [ ] Does the adaptive-agent stretch (M6) add real value, or is it scope creep? — decide after M5.
- [ ] Confirm QEMU board (`mps2-an385` vs `lm3s6965evb`) + toolchain (`arm-none-eabi-gcc`).

---

## 8. Glossary (quick reference)

- **WCET** — Worst-Case Execution Time (the Cᵢ above).
- **Jitter** — variation in when a periodic task actually runs vs. its ideal time.
- **Preemption** — higher-priority task interrupts a running lower one.
- **NVIC** — Cortex-M's Nested Vectored Interrupt Controller.
- **PendSV** — the low-priority exception used to trigger context switches.
- **Tickless idle** — stop the periodic tick when idle to save power (advanced).

---

## §9: EDF vs RMS Comparison (M6)

EDF (Earliest-Deadline-First) is a **dynamic**-priority scheduler: instead of
a fixed priority assigned at task-creation time (RMS), the task with the
nearest absolute deadline always runs next. This project adds it as a
second, compile-time-selectable ready-queue policy (`make SCHED=EDF`)
alongside the existing RMS priority array (`make SCHED=RMS`, the default).

**Schedulability bounds:**
- RMS (fixed priority): `U <= N(2^(1/N) - 1)` — for N=2 tasks, `~0.8284`.
- EDF (dynamic priority): `U <= 1.0` — exact (necessary AND sufficient) when
  every task's deadline equals its period, which this demo uses throughout
  (unlike M5's constrained-deadline demo).

**Results** (two periodic tasks, PERIOD_A=50, PERIOD_B=80 ticks, deadline=period):

| Workload | U | RMS verdict | RMS runtime | EDF verdict | EDF runtime |
|---|---|---|---|---|---|
| 1 (low) | 0.51 | SCHEDULABLE | 0 misses | SCHEDULABLE | 0 misses |
| 2 (medium) | 0.92 | NOT SCHEDULABLE | TaskB misses | SCHEDULABLE | 0 misses |
| 3 (high) | 0.955 | NOT SCHEDULABLE | TaskB misses | SCHEDULABLE | 0 misses |
| 4 (overload) | 1.1625 | NOT SCHEDULABLE | TaskB misses | NOT SCHEDULABLE | misses |

Workloads 2 and 3 are the headline result: the exact same task set is
provably NOT SCHEDULABLE under RMS (and genuinely misses deadlines at
runtime, confirmed by hand via response-time analysis) while EDF schedules
it cleanly — EDF's dynamic priority reordering achieves strictly better
utilization than any fixed-priority assignment can, up to the full `U<=1`
ceiling. Workload 4 shows EDF isn't magic: above `U=1`, no scheduler of any
kind can meet every deadline (total demand exceeds available CPU capacity),
and EDF misses deadlines too.

**Caveat carried over from RMS (§4):** `edf_check()`'s `U<=1` test is only
an *exact* schedulability proof because this demo uses implicit deadlines
(`D=T`). With constrained deadlines (`D<T`, as M5's demo used), `U<=1` is
still necessary but not automatically sufficient — the runtime deadline-miss
counter remains the ground truth for that stricter case.

---

## §10: Interrupt-Driven ADC Driver + Environmental-Monitor Pipeline (M7, QEMU phase)

Every prior milestone's only I/O was a polling UART driver, which never
exercised the kernel under genuine interrupt pressure. M7 adds the
project's first interrupt-driven peripheral driver: CMSDK Timer0 (IRQ8)
fires every 20 ticks, standing in for "ADC conversion complete" (QEMU's
`mps2-an385` doesn't model a real ADC), and its ISR pushes a synthetic
sample into a new bounded-queue kernel primitive (`queue.h/.c`, built on
the existing semaphore + critical-section primitives).

Three tasks form a real producer/consumer pipeline under RMS fixed
priorities: **Sampler** (priority 0) drains the ADC queue and hands off to
**Filter** (priority 1), which computes a 3-sample moving average and
branches directly to a **bounded-latency GPIO alert** on a threshold
crossing (not queued through the lower-priority **Reporter**, priority 2,
which just logs over the existing UART driver). Two design corrections
were made during planning, before any implementation:

1. QEMU's GPIO peripherals on `mps2-an385` are unimplemented stub devices
   (`create_unimplemented_device`, confirmed in QEMU's own `hw/arm/mps2.c`)
   with no readable state — so the alert's bounded latency is proven by
   the firmware logging `tick_count` immediately before/after the GPIO
   write over UART, not by reading QEMU's peripheral state back.
2. Sampler does not use the M5/M6 periodic-task deadline-miss counter,
   since that mechanism assumes a task self-paces its own release via
   `block_until()` — it has no way to represent an externally
   interrupt-driven release. Sampler's real hard-deadline signal is each
   queue's `overflow_count`, a strictly more accurate proxy for an
   ISR-driven producer.

This milestone's real hardware port (real Pico, real ADC/GPIO registers)
remains a separate, not-yet-written spec — see CLAUDE.md's Next Steps.

---

*Next step: whiteboard Q&A on any section above, then scope the project (§7) before writing a plan.*
