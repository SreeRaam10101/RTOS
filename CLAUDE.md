# RTOS Project — Claude Memory

> From-scratch tiny RTOS, learned top-to-bottom on QEMU (ARM Cortex-M).
> Builds on prior xv6-x64 scheduler work (`/Users/raam/Desktop/Notes/OS`).
> Learning doc: `rtos-whiteboard.md` (concepts §0–§8, milestone arc §6b). Session logs: `CC-Session-Logs/`.
> Architecture spec (M0–M5, approved): `docs/superpowers/specs/2026-07-09-rtos-core-architecture-design.md`.
> M0 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-09-m0-bare-metal-boot.md`.
> M1 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m1-context-switch.md`.
> M2 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m2-preemptive-scheduler.md`.
> M3 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m3-blocking-primitives.md`.

## Status

**Stage:** M3 implemented, reviewed (all 3 tasks + final whole-branch, all independently re-verified via build/QEMU/gdb), and merged to `master`. Worktree cleaned up. Next: write the M4 (priority inheritance) implementation plan via `writing-plans` — see the M4-handoff notes below first, they're required pre-work, not optional polish.

| Milestone | Description | Status |
|-----------|-------------|--------|
| M0 | Bare-metal boot — one task prints forever | ✅ done, merged (`rtos/`: linker.ld, boot.s, uart.{h,c}, main.c; `tests/m0_test.sh` passes) |
| M1 | Context switch — 2 tasks alternate via PendSV (**the core**) | ✅ done, merged (`rtos/`: kernel.{h,c}, pendsv.s, boot.s/Makefile edits; `tests/m1_test.sh` — superseded by later demos, see note below) |
| M2 | Preemptive scheduler — SysTick tick + priority ready-queue | ✅ done, merged (`rtos/`: `ready_queue[]` in kernel.c, scheduler.c, systick.c, pendsv.s renamed `kernel_start`→`scheduler_start`; `tests/m2_test.sh` — superseded by M3's demo, see note below) |
| M3 | Blocking primitives — delay(ticks), semaphore, mutex | ✅ done, merged (`rtos/`: ready-queue refactored to remove-on-select/re-enqueue-on-preempt, `critical_enter`/`critical_exit`, new `sync.{h,c}` — delay/semaphore/mutex with highest-priority wake + direct ownership hand-off; `tests/m3_test.sh` passes, asserting an *exact* 1,2,3,...,N printed sequence — a stronger check than any prior milestone) |
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
- **M2's SysTick pends PendSV unconditionally every tick** (not only "when a higher-priority task becomes ready," as the spec's literal wording suggested) — matches how real RTOSes do time-sliced preemption (FreeRTOS's `configUSE_TIME_SLICING`).
- **M3 resolved M2's ready-queue fragility** by changing what "in `ready_queue`" *means*: a task is there iff `TASK_READY` (not running, not blocked). `scheduler_next()` removes the selected task from `ready_queue` and marks it `TASK_RUNNING`; if the outgoing task is still `TASK_RUNNING` (preempted, not blocked) it's re-enqueued at the tail. This made M2's rotation trick and its "head == whoever just ran" fragile assumption unnecessary — verified byte-for-byte behavior-preserving against M2's exact test before any new blocking logic was added on top.
- **Critical sections are now load-bearing:** `critical_enter()`/`critical_exit()` (PRIMASK save/restore, nestable) wrap every mutation of `ready_queue`, the delay-list, and any semaphore/mutex `wait_queue` — SysTick (priority 0) can preempt Thread-mode code at any instruction boundary now that its handler also touches the delay-list.
- **Semaphore/mutex wake policy is "highest priority," not FIFO** — `sem_post()`/`mutex_unlock()` scan the `wait_queue` for the lowest `priority` value, matching the same real-time-safety invariant used everywhere else in this kernel (highest-ready-priority always wins).
- **Mutex hand-off never exposes a `count=1` window** — `mutex_unlock()` either directly assigns `owner` to the woken waiter (count stays 0) or resets `count=1`/`owner=0` if nobody was waiting. This closes a double-acquisition race and is exactly the `owner` invariant M4's priority-inheritance check needs to be reliable.
- **`tests/m1_test.sh` and `tests/m2_test.sh` are now stale/expected-to-fail** — each milestone's `main.c` demo is deliberately overwritten by the next; the milestone's test script is a point-in-time snapshot proven at merge time, not a permanent regression suite. `tests/m3_test.sh` is the only one still expected to pass on current `master`.

### M4 handoff (required pre-work, from M3's final review — not yet acted on)

- **`ready_remove()`'s unconditional `tcb->next = 0` is a latent corruption risk once M4 reuses it.** Today the only caller (`scheduler_next()`) always passes a TCB it just read from the ready-queue head, so the bug is unreachable. M4's `boost_priority()`/`restore_priority()` will want to relocate a mutex owner that is frequently `TASK_RUNNING` (in no queue) or `TASK_BLOCKED` (on a *different* wait_queue) — calling `ready_remove()` on such a TCB would clobber `tcb->next` and corrupt whatever list it's actually on. **Fix before M4 uses it:** move `tcb->next = 0` inside the `if (*pp == tcb)` block so it's a no-op when the TCB isn't actually in that ready-queue slot.
- **`boost_priority()` must branch on `owner->state`, not always relocate.** The spec's M4 sketch describes it as a blanket "ready-queue relocation," but the mutex owner being boosted is typically `TASK_RUNNING` or `TASK_READY` (sometimes `TASK_BLOCKED` on a nested mutex) — only the `TASK_READY` case needs an actual `ready_remove`+`ready_enqueue` move. For `TASK_RUNNING`/`TASK_BLOCKED`, just mutate `tcb->priority`; the next natural `ready_enqueue()` (on preemption or wake) will already use the updated value.
- **M3's mutex hand-off path has zero test coverage.** `tests/m3_test.sh`'s producer never touches `counter_mutex` — only the consumer does — so `mutex_unlock()`'s hand-off branch (`wait_queue` non-empty) and `wait_queue_pop_highest()` with >1 waiter are hand-verified but never exercised by a runnable test. M4's classic 3-task inversion demo (Low holds the mutex, High blocks on it, Medium preempts) will finally drive this path — treat that demo as also being M3's missing regression test, not just M4's teaching payoff.

## Next Steps

1. Write the M4 (priority inheritance) implementation plan via `writing-plans` — incorporate the three M4-handoff items above (fix `ready_remove()`'s clobber bug first, design `boost_priority()`/`restore_priority()` to branch on `owner->state`, and treat the classic 3-task inversion demo as the mutex hand-off's first real test). Per the spec, M4 extends `sync.c` only — no new files.
2. Execute M4 the same way (worktree → subagent-driven-development → final review → merge).
3. Toolchain is fully installed: `qemu-system-arm`, `arm-none-eabi-gcc`, `arm-none-eabi-gdb` all confirmed working since M0.

## Conventions

- Keep explanations **brief** (user preference).
- Capture reusable learning in `rtos-whiteboard.md` so it travels to claude.ai.
- Still in brainstorming; do not scaffold/implement until design approved.
