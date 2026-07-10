# Session Log: 02-07-2026 00:58 - rtos-whiteboard-planning

## Quick Reference (for AI scanning)
**Confidence keywords:** RTOS, QEMU, Cortex-M, ARM, scheduler, RMS, EDF, priority-inheritance, priority-inversion, schedulability, xv6, MLFQ, whiteboard, brainstorming, real-time, determinism, PendSV, NVIC
**Projects:** RTOS (new, `/Users/raam/Desktop/Notes/LLM/RTOS`), builds on prior xv6-x64 OS work (`/Users/raam/Desktop/Notes/OS`)
**Outcome:** Scoped a QEMU-based ARM Cortex-M RTOS learning project; produced a whiteboard learning doc (`rtos-whiteboard.md`); deferred concrete design/plan pending user's §7 decisions.

## Decisions Made
- **Platform: QEMU-emulated ARM Cortex-M** (NOT a physical board). User wants to build solid software/QEMU experience first. Mirrors prior xv6+QEMU workflow.
- **Whiteboard/learning session BEFORE any implementation plan.** User explicitly wants to learn concepts first, not scope+build yet.
- **Keep explanations brief** and captured in a `.md` file so the user can continue learning on claude.ai.
- Recommended (not yet confirmed) target board: `mps2-an385` (Cortex-M3) or `lm3s6965evb`; toolchain `arm-none-eabi-gcc` + `gdb`.
- Recommended project shape: from-scratch tiny RTOS kernel in layers (context switch → priority preemptive scheduler → blocking primitives → priority inheritance → RMS/EDF + deadline harness → optional adaptive agent).

## Key Learnings
- **RTOS vs xv6 core shift:** xv6 optimizes throughput/fairness (fastest on average); RTOS optimizes determinism (never late, worst-case). Same machinery, opposite success metric.
- **Cortex-M is easier than x86-64 for this:** no MMU/page tables, hardware auto-stacks registers on interrupt entry, clean NVIC, context switch via PendSV (~15 lines asm). Energy goes to scheduling logic, not architecture arcana.
- **The RTOS canon (3 ideas not yet done in xv6):** (a) priority-based preemptive scheduling — highest-ready-priority always runs, priorities fixed (vs MLFQ decay); (b) priority inversion + inheritance (Mars Pathfinder story); (c) bounded interrupt latency.
- **Priority assignment:** RMS (shorter period → higher priority; static, provable by hand) vs EDF (nearest deadline runs; dynamic, better utilization, harder).
- **Schedulability bounds:** RMS `U = Σ(Cᵢ/Tᵢ) ≤ N(2^(1/N)−1)` (~0.69 asymptotic); EDF schedulable iff `U ≤ 1.0`.
- **QEMU is legit for learning** because deadline logic is measured in tick counts (not nanoseconds) — same tick-based reasoning as the user's xv6 `create_tick`/`first_run_tick` response-time work.
- User's prior xv6 work reviewed: term project = "Agentic Hybrid MLFQ+SJF Scheduler" (submitted); HW4 VGA/keyboard driver, HW5 signals, HW6 mmap, HW7 doubly-indirect FS. Unfinished xv6 stretch = RL epsilon-greedy bandit — flagged as a natural home in the RTOS adaptive-agent stretch.

## Files Modified
- `/Users/raam/Desktop/Notes/LLM/RTOS/rtos-whiteboard.md`: **created** — concise RTOS learning doc (§0 one-line diff, §1 real-time defs, §2 Cortex-M vs x86-64 table, §3 RTOS canon, §4 schedulability math, §5 QEMU setup, §6 layered project arc, §7 open questions, §8 glossary). Portable for continuing on claude.ai.
- `/Users/raam/Desktop/Notes/LLM/RTOS/CC-Session-Logs/02-07-2026-00_58-rtos-whiteboard-planning.md`: this session log.

## Pending Tasks
Open scoping questions (from `rtos-whiteboard.md` §7) to settle with user before writing a plan:
- [ ] Full from-scratch kernel, or a real-time scheduler on a minimal existing base?
- [ ] RMS (simpler, provable) or EDF (harder, better utilization) as target scheduler?
- [ ] How to measure determinism in QEMU (tick-based deadline-miss counter)?
- [ ] Does the adaptive-agent stretch (revive the xv6 RL bandit) add value, or is it scope creep?
- [ ] Confirm QEMU board + toolchain choice.
- [ ] Next unfired step: whiteboard Q&A on any section, THEN scope §7, THEN writing-plans skill.

## Key Exchanges
- User asked Claude to review prior OS projects via CLAUDE.md files only → found 2 (root `OS/CLAUDE.md` + `OS/FTP/CLAUDE.md`), summarized xv6 assignments + agentic scheduler term project.
- User (no fixed idea) asked what RTOS projects fit their background → Claude proposed 4 ranked ideas, recommended real-time scheduler (#1) or from-scratch kernel (#2).
- User chose QEMU-only + whiteboard-first + brief + write-to-md → Claude wrote `rtos-whiteboard.md`.
- Brainstorming skill invoked but intentionally held at learning stage (HARD-GATE: no implementation until design approved).

## Custom Notes
None (user away during prompts; proceeded on best judgment).

---

## Quick Resume Context
Starting a new RTOS project in `/Users/raam/Desktop/Notes/LLM/RTOS`, target = QEMU-emulated ARM Cortex-M (no physical board yet). A whiteboard learning doc `rtos-whiteboard.md` already exists covering RTOS fundamentals mapped to the user's prior xv6-x64 scheduler experience. Next step is user-driven: whiteboard Q&A on any concept, then settle the §7 scoping questions (from-scratch vs base, RMS vs EDF, determinism measurement, adaptive-agent stretch), then invoke writing-plans. Keep explanations brief; user continues some learning on claude.ai.

---

## Raw Session Log

**User:** Asked Claude to review prior OS-folder projects under `/Users/raam/Desktop/Notes`, reading only CLAUDE.md files (skip folders lacking one), to get a brief picture before landing on an RTOS project.

**Assistant:** Listed Notes dir structure; found two CLAUDE.md files in OS tree (`OS/CLAUDE.md`, `OS/FTP/CLAUDE.md`). Summarized:
- Course: CS 461 / ECE 461, xv6-x64 on QEMU.
- Assignments: HW4 (VGA Mode 13h driver, CGA save/restore, raw keyboard, Mario bonus), HW5 (Unix signals: trapframe hijacking, sys_sigret/sys_fgproc/SIGINT, sys_alarm + signal disposition), HW6 (mmap eager+lazy), HW7 (doubly-indirect FS blocks: bmap/itrunc, max file 16523 blocks).
- Term project: "Agentic Hybrid MLFQ+SJF Scheduler for xv6" — MLFQ+SJF replacing round-robin + autonomous userspace agent (sagent.c) reading kernel telemetry via getschedstats/setschedparam/register_agent; fully submitted (submission/main, PDF ~2026-05-01); result MLFQ beats RR ~6% (p<0.0001), agent adds ~noise; K&R C, strict lock ordering; RL bandit stretch (Phase S) NOT started.

**User:** "I've no idea, freshly starting; based on my past projects, what similar RTOS projects can I build — just wanted to discuss and learn."

**Assistant:** Invoked superpowers:brainstorming. Explained core shift (throughput→determinism). Proposed 4 ranked ideas: (1) real-time scheduler remade for deadlines [strongest fit, reuses eval harness + RL bandit home]; (2) from-scratch tiny RTOS kernel [deepest learning]; (3) priority inversion→inheritance demo [small, teachable]; (4) real-time sensor pipeline on FreeRTOS/Zephyr. Recommended #1 or #2. Asked platform question (real board / QEMU bare metal / RTOS lib / help me pick). User away → Claude recommended QEMU-emulated ARM Cortex-M with reasoning (mirrors xv6 workflow; Cortex-M simpler; tick-based timing survives emulation); outlined 6-layer scope.

**User:** "Don't want physical board yet; build solid QEMU experience first; want a whiteboard session before plans; keep explanations brief; write to a .md file so I can continue on claude.ai."

**Assistant:** Confirmed QEMU + whiteboard-first. Created `rtos-whiteboard.md` (§0–§8 as described above). Offered to start whiteboard Q&A or continue on claude.ai; stated no design doc until §7 decided.

**User:** Ran `/compress`.

**Assistant:** Ran compress workflow; user away during preserve/topic prompts; proceeded with all sections preserved, topic `rtos-whiteboard-planning`, saved this log to project `CC-Session-Logs/`.
