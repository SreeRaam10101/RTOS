# Tests

Each `tests/mN_test.sh` is a **point-in-time regression check** for milestone
N's demo (`rtos/main.c` as it existed at milestone N's merge commit) — not a
permanent suite where all scripts are meant to pass simultaneously against
the current tree.

This project grows one kernel by replacing `main.c` at every milestone: each
milestone boots and demos a new capability, then the next milestone's demo
overwrites it. So at any given commit, only the **latest** milestone's test
script is expected to pass against the current `rtos/build/kernel.elf`.
Right now that's `tests/m7_test.sh`. `tests/m0_test.sh` through
`tests/m6_test.sh` failing here is expected, not a regression.

To verify an earlier milestone's test:
1. `git log --oneline` to find that milestone's merge commit.
2. Check it out in a scratch worktree (e.g. `git worktree add ../scratch <sha>`).
3. Rebuild there (`cd rtos && make clean && make`).
4. Run that milestone's script from the scratch worktree.

## Milestones

| Script | Milestone | Demo |
|--------|-----------|------|
| `m0_test.sh` | M0 | Bare-metal boot — one task prints forever |
| `m1_test.sh` | M1 | Context switch — 2 tasks alternate via PendSV |
| `m2_test.sh` | M2 | Preemptive scheduler — SysTick tick + priority ready-queue |
| `m3_test.sh` | M3 | Blocking primitives — delay(ticks), semaphore, mutex |
| `m4_test.sh` | M4 | Priority inheritance — build inversion, then fix |
| `m5_test.sh` | M5 | Real-time layer — periodic tasks + deadline-miss counter (RMS capstone) |
| `m6_test.sh` | M6 | EDF + RMS comparison — verify both schedulability and runtime miss behavior |
| `m7_test.sh` | M7 | Interrupt-driven ADC + environmental-monitor pipeline (Sampler/Filter/Reporter, bounded-latency GPIO alert) |
