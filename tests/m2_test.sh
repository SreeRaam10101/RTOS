#!/usr/bin/env bash
set -uo pipefail

ELF="rtos/build/kernel.elf"
LOG="rtos/build/m2_output.log"

mkdir -p rtos/build
qemu-system-arm -M mps2-an385 -kernel "$ELF" -nographic > "$LOG" 2>&1 &
QEMU_PID=$!
sleep 3
kill "$QEMU_PID" 2>/dev/null
wait "$QEMU_PID" 2>/dev/null

CLEAN_LOG=$(tr -d '\r' < "$LOG" 2>/dev/null)

COUNT_A=$(echo "$CLEAN_LOG" | grep -c "^task_a$")
COUNT_A=${COUNT_A:-0}
COUNT_B=$(echo "$CLEAN_LOG" | grep -c "^task_b$")
COUNT_B=${COUNT_B:-0}
COUNT_LOW=$(echo "$CLEAN_LOG" | grep -c "^task_low$")
COUNT_LOW=${COUNT_LOW:-0}

echo "task_a=$COUNT_A task_b=$COUNT_B task_low=$COUNT_LOW"

FAIL=0

if [ "$COUNT_A" -lt 5 ] || [ "$COUNT_B" -lt 5 ]; then
    echo "FAIL: expected task_a and task_b to each run >=5 times (SysTick-driven round robin), got task_a=$COUNT_A task_b=$COUNT_B"
    FAIL=1
fi

HIGHER=$COUNT_A
LOWER=$COUNT_B
if [ "$COUNT_B" -gt "$COUNT_A" ]; then
    HIGHER=$COUNT_B
    LOWER=$COUNT_A
fi
if [ "$LOWER" -gt 0 ] && [ $((HIGHER)) -gt $((LOWER * 2)) ]; then
    echo "FAIL: task_a/task_b counts too imbalanced (task_a=$COUNT_A task_b=$COUNT_B) -- not round-robining"
    FAIL=1
fi

if [ "$COUNT_LOW" -ne 0 ]; then
    echo "FAIL: task_low ran $COUNT_LOW times but should be fully starved while task_a/task_b (higher priority) are always ready"
    FAIL=1
fi

if [ "$FAIL" -eq 1 ]; then
    cat "$LOG"
    exit 1
fi

echo "PASS: task_a and task_b round-robin via SysTick (task_a=$COUNT_A, task_b=$COUNT_B), task_low correctly starved (0 runs)"
exit 0
