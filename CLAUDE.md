# RTOS Project — Claude Memory

> From-scratch tiny RTOS, learned top-to-bottom on QEMU (ARM Cortex-M).
> Builds on prior xv6-x64 scheduler work (`/Users/raam/Desktop/Notes/OS`).
> Learning doc: `rtos-whiteboard.md` (concepts §0–§9, milestone arc §6b). Session logs: `CC-Session-Logs/`.
> Architecture spec (M0–M5, approved): `docs/superpowers/specs/2026-07-09-rtos-core-architecture-design.md`.
> M0 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-09-m0-bare-metal-boot.md`.
> M1 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m1-context-switch.md`.
> M2 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m2-preemptive-scheduler.md`.
> M3 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m3-blocking-primitives.md`.
> M4 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m4-priority-inheritance.md`.
> M5 implementation plan (complete, merged): `docs/superpowers/plans/2026-07-10-m5-real-time-layer.md`.

## Status

**Stage:** 🎉 **M0–M5 spec scope is complete**, and the repo is now **public on GitHub** (`github.com/SreeRaam10101/RTOS`) with passing CI (`.github/workflows/ci.yml` — installs `gcc-arm-none-eabi`+`qemu-system-arm` via apt, builds, runs `tests/m5_test.sh`; verified green by actually watching the run, not just adding the file). A root `README.md` (human-facing, distinct from this file) was added since the repo went public. The from-scratch RTOS boots, context-switches, preempts by priority, blocks/wakes tasks, fixes priority inversion via inheritance, and runs periodic real-time tasks with a schedulability proof and a runtime deadline-miss counter. `tests/README.md` explains that only the latest milestone's test script (currently `tests/m5_test.sh`) is expected to pass against current `master` — see it and the "stale tests" note below.

**Companion project:** `/Users/raam/Desktop/Notes/LLM/FreeRTOS-QEMU` (also public, own CLAUDE.md) — gets the *real* FreeRTOS kernel running on the same QEMU board, proving the complementary "work inside a production kernel" skill. Both bootstrap+demo tasks complete, final-reviewed, and CI'd there too.

**No open next step is queued for THIS project** — M6 (adaptive agent / EDF comparison) and M7 (Pico hardware port) are explicitly out of scope for the current spec and would need their own brainstorming/design pass before any implementation plan. The active roadmap item right now is **Zephyr** (a second production RTOS, in the sibling `FreeRTOS-QEMU` repo or a new one), deferred until the FreeRTOS work was done.

| Milestone | Description | Status |
|-----------|-------------|--------|
| M0 | Bare-metal boot — one task prints forever | ✅ done, merged |
| M1 | Context switch — 2 tasks alternate via PendSV (**the core**) | ✅ done, merged |
| M2 | Preemptive scheduler — SysTick tick + priority ready-queue | ✅ done, merged |
| M3 | Blocking primitives — delay(ticks), semaphore, mutex | ✅ done, merged |
| M4 | Priority inheritance — build inversion, then fix (**the payoff**) | ✅ done, merged |
| M5 | Real-time layer — periodic tasks + deadline-miss counter (RMS capstone) | ✅ done, merged (`rtos/`: `block_until()` extracted from `delay()`, new `rtos.{h,c}` — `periodic_task_create()`/`task_wait_for_release()`/`periodic_get_miss_count()`/`rms_check()`, all integer-arithmetic, no `systick.c` changes needed; `tests/m5_test.sh` proves RMS reports SCHEDULABLE while a deliberately-overrunning task still racks up real deadline misses — the static check and the runtime counter check two different things on purpose) |
| M6 | EDF vs RMS comparison — `SCHED=RMS`/`EDF` compile-time flag, workload utilization sweep, deadline-miss counter as proof | ✅ done, merged |
| M7 | Stretch — migrate QEMU → real Raspberry Pi Pico | ⏳ needs its own design pass first |

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
- **Mutex hand-off never exposes a `count=1` window** — `mutex_unlock()` either directly assigns `owner` to the woken waiter (count stays 0) or resets `count=1`/`owner=0` if nobody was waiting. This closed a double-acquisition race and turned out to be exactly the `owner` invariant M4's priority-inheritance check needed.
- **M4 resolved the two M3-handoff bugs** (`ready_remove()`'s unconditional clobber; `boost_priority()`/`restore_priority()` correctly branch on `owner->state` — only relocate in `ready_queue` when `TASK_READY`, otherwise just mutate `tcb->priority` in place) and closed the mutex-hand-off coverage gap via the inversion demo (Low genuinely gets stuck without the fix — `Low: releasing` never prints — and escapes with it). Priority inheritance uses a single compile-time flag (`PRIORITY_INHERITANCE_ENABLED`) to build two ELFs from one source tree — a clean single-variable "prove it once broken, once fixed" experiment.
- **A test that only checks the "boost" half of inheritance can pass even if "restore" is completely broken** — a permanently-stuck-boosted task would silently corrupt the fixed-priority ordering everything downstream depends on. Fixed by asserting priority is always restored to base after unlock, verified to have teeth by deliberately breaking `restore_priority()` and confirming the test then fails. General lesson: an asymmetric test (checks the "on" transition but not the "off" one) is a common way to ship a real bug behind a green suite.
- **`delay()` was refactored (M5) into a thin wrapper over `block_until(absolute_tick)`** — this let periodic-task releases reuse the exact SysTick-driven delay-list mechanism built in M3, with zero `systick.c` changes. Whenever a "relative time" primitive already exists, check whether extracting its "absolute time" core enables the next feature for free before building parallel infrastructure.
- **RMS's Liu & Layland bound assumes deadline=period (implicit deadlines)** — this project's M4/M5 demos use deadline<period (constrained deadlines), so `rms_check()` (which only ever looks at WCET/period, never `deadline_ticks`) technically proves "schedulable if deadlines equaled periods," not that the shorter real deadlines are met. Documented explicitly (not silently papered over) in `rtos.c`'s `rms_check()` doc comment and `rtos-whiteboard.md` §4 — the runtime deadline-miss counter is what actually enforces the stricter real deadlines. This ended up being a *feature* of the M5 demo (static check says schedulable; runtime counter still catches real violations) rather than a bug, once made explicit.
- **`tests/m0_test.sh` through `tests/m4_test.sh` are stale/expected-to-fail on current `master`** — each milestone's `main.c` demo is deliberately overwritten by the next, so only the latest milestone's test script passes against current `HEAD`. This is now documented in `tests/README.md` (added after the M5 final review flagged a 5/6-red test suite as a bad first impression for anyone new to the repo) — to verify an earlier milestone's test, check out that milestone's merge commit in a scratch worktree and rebuild there.
- **A fix subagent once committed directly to `master`** (not its assigned worktree) because the target file (`rtos-whiteboard.md`) lives outside `rtos/`, and included the `Co-Authored-By: Claude` trailer the user had already asked to omit — caught and amended after the fact. Lesson: when dispatching a subagent to edit files that might live outside the current worktree's tracked subtree, explicitly state which branch/location to commit to and restate the no-co-author-trailer convention in the dispatch prompt — don't assume a fresh subagent inherits project conventions it was never told.
- **CI on GitHub Actions is genuinely cheaper on Linux than local macOS dev was.** Ubuntu's `apt-get install gcc-arm-none-eabi qemu-system-arm` gives a complete toolchain (with newlib) in one step — no toolchain-gap surprises like the sibling FreeRTOS-QEMU project hit locally on macOS (see that project's CLAUDE.md). Verify CI actually passes by watching the run (`gh run watch --exit-status`), not just by confirming the workflow file parses/uploads.
- **Public-repo readiness needs a human-facing `README.md` distinct from `CLAUDE.md`.** `CLAUDE.md`/`rtos-whiteboard.md` are internal working docs (session-continuity, decision log) — a recruiter or engineer clicking the GitHub link wants a short "what is this, how do I build/run it" doc up front, with a CI badge as a cheap trust signal.
- **M6's WCET-tracking lesson** — initial `busy_wait()` design used wall-clock tick_count, masking preemption intervals so RMS-vs-EDF runtime misses never surfaced; fixed by adding `tcb->remaining_wcet_ticks`, decremented only by SysTick_Handler when a task is current. Separately: 'highest-priority never misses' is RMS-specific (fixed priority); EDF's dynamic reordering lets both tasks accumulate misses under overload, confirmed in workload-4 test.

## Next Steps

The M0–M5 spec is complete and the repo is public with passing CI — there is no queued implementation work on THIS project. If the user wants to continue here specifically:
1. M6 (adaptive RL-bandit agent, or EDF-vs-RMS comparison) and M7 (Raspberry Pi Pico hardware port) both need their own `brainstorming` → spec → `writing-plans` cycle first — they were explicitly out of scope for the approved M0–M5 spec, not just "not started yet."
2. Toolchain is fully installed: `qemu-system-arm`, `arm-none-eabi-gcc`, `arm-none-eabi-gdb` all confirmed working since M0.
3. The worktree → subagent-driven-development → final review → merge pattern used for M0–M5 worked well each time and is the natural template for whatever comes next.

**Active roadmap (career-development context, spans multiple repos):** after this project (from-scratch RTOS) and the FreeRTOS-QEMU companion, the next planned step is **Zephyr** — a second production RTOS, deliberately deferred until FreeRTOS was done so as not to learn two new build systems (CMake/Kconfig/devicetree vs FreeRTOS's plain Makefile) at once. See `/Users/raam/Desktop/Notes/LLM/FreeRTOS-QEMU/CLAUDE.md` for the fuller embedded/firmware roadmap this project sits inside (real Pico hardware, peripheral drivers, tickless idle, MISRA-C — all still pending, roughly in that priority order).

## Conventions

- Keep explanations **brief** (user preference).
- Capture reusable learning in `rtos-whiteboard.md` so it travels to claude.ai.
- Still in brainstorming; do not scaffold/implement until design approved.
