#!/usr/bin/env bash
set -uo pipefail

ELF="rtos/build/kernel.elf"
LOG="rtos/build/m1_output.log"

mkdir -p rtos/build
qemu-system-arm -M mps2-an385 -kernel "$ELF" -nographic > "$LOG" 2>&1 &
QEMU_PID=$!
sleep 3
kill "$QEMU_PID" 2>/dev/null
wait "$QEMU_PID" 2>/dev/null

COUNT0=$(tr -d '\r' < "$LOG" 2>/dev/null | grep -c "^task0$")
COUNT0=${COUNT0:-0}
COUNT1=$(tr -d '\r' < "$LOG" 2>/dev/null | grep -c "^task1$")
COUNT1=${COUNT1:-0}

echo "task0 count: $COUNT0, task1 count: $COUNT1"

if [ "$COUNT0" -lt 5 ] || [ "$COUNT1" -lt 5 ]; then
    echo "FAIL: expected both task0 and task1 to run >=5 times, got task0=$COUNT0 task1=$COUNT1"
    cat "$LOG"
    exit 1
fi

# Check the two counts are within 2x of each other -- evidence of round-robin
# alternation rather than one task running away while the other starves.
HIGHER=$COUNT0
LOWER=$COUNT1
if [ "$COUNT1" -gt "$COUNT0" ]; then
    HIGHER=$COUNT1
    LOWER=$COUNT0
fi

if [ $((HIGHER)) -gt $((LOWER * 2)) ]; then
    echo "FAIL: task0/task1 counts too imbalanced (task0=$COUNT0 task1=$COUNT1) -- not alternating"
    exit 1
fi

echo "PASS: task0 and task1 both ran and alternated (task0=$COUNT0, task1=$COUNT1)"
exit 0
