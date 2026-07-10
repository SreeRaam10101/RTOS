# RTOS Project — Claude Memory

> From-scratch tiny RTOS, learned top-to-bottom on QEMU (ARM Cortex-M).
> Builds on prior xv6-x64 scheduler work (`/Users/raam/Desktop/Notes/OS`).
> Learning doc: `rtos-whiteboard.md` (concepts §0–§8, milestone arc §6b). Session logs: `CC-Session-Logs/`.
> Architecture spec (M0–M5, approved): `docs/superpowers/specs/2026-07-09-rtos-core-architecture-design.md`.
> M0 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-09-m0-bare-metal-boot.md`.
> M1 implementation plan (implemented, reviewed, NOT yet merged): `docs/superpowers/plans/2026-07-10-m1-context-switch.md`.

## Status

**Stage:** M1 implemented across 2 tasks (both task-reviews approved) and the final whole-branch review returned **"ready to merge, with one fix"** — not yet applied, not yet merged. Work is sitting in the open worktree at `.claude/worktrees/m1-context-switch` (branch `worktree-m1-context-switch`, HEAD `dda0777`), NOT on `master` yet.

**Resume here:** the final reviewer's Bash tool was down for its entire session (~40 failed attempts) — it verified correctness via independent static hand-trace only, never actually ran `make`/QEMU/gdb. Before merging: (1) apply the stack-alignment fix below, (2) actually run `cd rtos && make clean && make && cd .. && ./tests/m1_test.sh` and the gdb `current_tcb`-alternation check from the plan's Task 2 Step 7 to confirm the hand-trace's predictions hold, (3) then merge via `finishing-a-development-branch`.

**The one recommended fix (Important, not blocking M1's cooperative demo but load-bearing for M2):** `rtos/kernel.c`'s `task_stacks` array has no alignment attribute (`static uint32_t task_stacks[MAX_TASKS][TASK_STACK_WORDS];`, only 4-byte-aligned via `.bss`'s `ALIGN(4)`). AAPCS/ARMv7-M require 8-byte stack alignment at call boundaries; `task_create()` hand-builds the initial exception frame so hardware's STKALIGN auto-padding never corrects it. Harmless today (cooperative-only), but M2 adds SysTick-driven preemption which hardware-stacks onto whatever PSP a task has — misalignment becomes a live bug then. Fix: add `__attribute__((aligned(8)))` to the `task_stacks` declaration.

| Milestone | Description | Status |
|-----------|-------------|--------|
| M0 | Bare-metal boot — one task prints forever | ✅ done, merged (`rtos/`: linker.ld, boot.s, uart.{h,c}, main.c; `tests/m0_test.sh` passes) |
| M1 | Context switch — 2 tasks alternate via PendSV (**the core**) | 🔶 implemented + reviewed, **not merged** — see Status above |
| M2 | Preemptive scheduler — SysTick tick + priority ready-queue | ⏳ |
| M3 | Blocking primitives — delay(ticks), semaphore, mutex | ⏳ |
| M4 | Priority inheritance — build inversion, then fix (**the payoff**) | ⏳ |
| M5 | Real-time layer — periodic tasks + deadline-miss counter (RMS capstone) | ⏳ |
| M6 | Stretch — adaptive agent (revive xv6 RL-bandit) OR add EDF + compare | ⏳ |
| M7 | Stretch — migrate QEMU → real Raspberry Pi Pico | ⏳ |

## Key Decisions

| Decision | Reason |
|----------|--------|
| **QEMU-emulated Cortex-M**, no physical board (yet) | Build solid software/QEMU experience first; mirrors xv6+QEMU workflow |
| **Option 2: from-scratch kernel** (not scheduler-on-a-base) | Deepest top-to-bottom learning; user owns every line |
| **RMS capstone — confirmed** | Hand-provable bound cements "why real-time works"; EDF excluded entirely (not even M6) |
| Cortex-M target (not x86-64) | No MMU, HW auto-stacks registers, clean NVIC → energy on scheduling logic |
| Hardware = later/cheap/optional (Pico ~$6 + probe ~$12) | Not a prerequisite; QEMU-first decision stands |
| **QEMU board: `mps2-an385`** (Cortex-M3) | Better documented / actively maintained in QEMU than `lm3s6965evb` |
| **Ready-queue: priority array + linear scan** (not bitmap+CLZ, not sorted list) | O(1) enqueue/dequeue, easy to inspect in gdb; O(N_priorities) scan is fine at learning scale |
| **Code layout: layered modules**, one file per concern (`boot.s`, `pendsv.s`, `kernel.c`, `scheduler.c`, `systick.c`, `sync.c`, `rtos.c`, `uart.c`, `main.c`) | Each milestone touches a small, predictable diff; isolates asm from C |
| Design scope = **M0–M5 only** for this spec | M6 (adaptive agent)/M7 (Pico port) depend on unknowns not yet learned; separate spec later |

## Patterns / Insights

- **Toolchain is identical QEMU → real Pico** — `arm-none-eabi-gcc` + `gdb` unchanged; only OpenOCD replaces QEMU's built-in gdb server. Re-targeting mostly = linker addresses + startup/clock init.
- **M1 (context switch) is the whole game** — once 2 tasks swap via PendSV, rest is bookkeeping. Cortex-M simpler than xv6 `swtch` (HW stacks half the registers).
- **M4 (priority inversion→inheritance) is the teaching payoff** — inversion invisible until constructed; Mars Pathfinder story; best demo + writeup.
- **QEMU is legit for real-time learning** — deadline logic counted in ticks, not ns (same tick-based reasoning as xv6 `create_tick`/`first_run_tick`).
- **"Raspberry Pi" ≠ "Pi Pico"** — the RTOS target is the **Pico** (RP2040/RP2350, Cortex-M microcontroller), NOT the Linux SBC (Pi 4/5, has MMU/runs Linux).
- **UART0 on mps2-an385 is CMSDK APB UART @ `0x40004000`** — DATA/STATE/CTRL/BAUDDIV at `0x00/0x04/0x08/0x10`; SYSCLK = 25MHz. Confirmed from QEMU's own source (`hw/arm/mps2.c`, `hw/char/cmsdk-apb-uart.c`), not guessed — wrong peripheral addresses fail silently (no compiler error), so verify against source/datasheet, not memory.
- **Linker script `.data`/`.bss` boundaries need explicit `. = ALIGN(4);`** before `_edata`/`_ebss` — the 4-byte-stride copy/zero loop in `Reset_Handler` silently overruns by up to 3 bytes if a section's size isn't a multiple of 4. Invisible in M0 (no globals yet); added proactively before M1 introduces TCBs/globals.
- **M1's context-switch frame contract (verified correct via independent hand-trace):** `task_create()` builds a fake exception frame top-down on a new task's stack — high→low address: `xPSR(0x01000000), PC(entry), LR(0xFFFFFFFD), R12, R3, R2, R1, R0`, then reserves 8 more words below for `R4-R11`. `tcb->sp` points at the bottom (R4). `PendSV_Handler`'s `stmdb`/`ldmia {r4-r11}` and `kernel_start`'s manual bootstrap unwind both consume exactly this layout — first-run-via-bootstrap and resume-via-real-switch converge on the same frame, which is the standard minimal-RTOS trick (same idiom FreeRTOS/ChibiOS use).
- **`bl scheduler_next` inside `PendSV_Handler` clobbers `lr`**, which holds `EXC_RETURN` (`0xFFFFFFFD`, needed for the final `bx lr`) — must be preserved with `push {lr}` / `pop {lr}` bracketing the call. Easy to miss, silent corruption if missed.
- **Two of my own plan's test-script lines were buggy** (`grep -c ... || echo 0` double-prints on zero-match, corrupting numeric comparisons; `^task0$` anchors don't match because `uart_puts()` emits `\r\n`) — caught and fixed by the M1 Task-2 implementer, confined to `tests/m1_test.sh`. Lesson: even carefully-derived plan code needs the same scrutiny as hand-written code: verify shell idioms (`|| echo 0` style fallbacks especially) rather than assuming plan text is correct by construction.
- **Worktree workflow per milestone**: each milestone gets its own `EnterWorktree` (name matches milestone, e.g. `m1-context-switch`) branched from `master`. If plan/CLAUDE.md docs get committed to `master` *after* a worktree was already created, the worktree branch needs an explicit `git merge master` to pick them up — worktrees don't auto-sync.
- **User does not want `Co-Authored-By: Claude` in commit messages** for this project — confirmed after an explicit rejection during the `git init` commit.

## Next Steps

1. **Resume M1 finish-up** (see Status above): apply the 8-byte stack-alignment fix, actually run the build/test/gdb verification (not yet executed this session due to a tool outage), then merge M1 to `master` via `finishing-a-development-branch`. The worktree at `.claude/worktrees/m1-context-switch` is still open with the reviewed work — don't re-run the implementation tasks, don't re-do the two task-level reviews (already ✅ approved with hand-traced correctness).
2. After M1 merges: write the M2 (preemptive scheduler) implementation plan via `writing-plans`. Note for that plan: `scheduler_next()` needs to move from `kernel.c` (M1's placeholder, naive round-robin) to a new `scheduler.c` with the real priority-array design — `pendsv.s`/`kernel_start` should need zero changes (same C function signature), but `task_table`/`num_tasks` bookkeeping in `kernel.c` will need to be reworked into the `ready_queue[]` enqueue/dequeue model from the architecture spec.
3. Toolchain is fully installed: `qemu-system-arm`, `arm-none-eabi-gcc`, `arm-none-eabi-gdb` all confirmed working during M0.

## Conventions

- Keep explanations **brief** (user preference).
- Capture reusable learning in `rtos-whiteboard.md` so it travels to claude.ai.
- Still in brainstorming; do not scaffold/implement until design approved.
