#!/usr/bin/env bash
set -uo pipefail

ELF="rtos/build/kernel.elf"
LOG="rtos/build/m5_output.log"

mkdir -p rtos/build
qemu-system-arm -M mps2-an385 -kernel "$ELF" -nographic > "$LOG" 2>&1 &
QEMU_PID=$!
sleep 3
kill "$QEMU_PID" 2>/dev/null
wait "$QEMU_PID" 2>/dev/null

CLEAN_LOG=$(tr -d '\r' < "$LOG" 2>/dev/null)

RMS_LINE=$(echo "$CLEAN_LOG" | grep '^RMS check:')
echo "RMS line: $RMS_LINE"

FAIL=0

if ! echo "$RMS_LINE" | grep -q 'SCHEDULABLE$'; then
    echo "FAIL: expected RMS check to report SCHEDULABLE (not NOT SCHEDULABLE, and not missing entirely)"
    FAIL=1
fi
if echo "$RMS_LINE" | grep -q 'NOT SCHEDULABLE$'; then
    echo "FAIL: RMS check reported NOT SCHEDULABLE -- expected utilization (0.225) is well under the N=2 bound (0.8284)"
    FAIL=1
fi

A_MISSES=$(echo "$CLEAN_LOG" | grep '^PeriodicA:' | sed 's/.*misses=//')
A_BAD=$(echo "$A_MISSES" | grep -cv '^0$')
A_BAD=${A_BAD:-0}
A_COUNT=$(echo "$A_MISSES" | grep -c '^[0-9]')
A_COUNT=${A_COUNT:-0}

if [ "$A_COUNT" -lt 10 ]; then
    echo "FAIL: expected >=10 PeriodicA releases in ~3s (50ms period), got $A_COUNT"
    FAIL=1
fi
if [ "$A_BAD" -ne 0 ]; then
    echo "FAIL: PeriodicA (well-behaved task) reported a nonzero miss count at least once -- it should never miss its deadline"
    FAIL=1
fi

B_MISSES=$(echo "$CLEAN_LOG" | grep '^PeriodicB:' | sed 's/.*misses=//')
B_COUNT=$(echo "$B_MISSES" | grep -c '^[0-9]')
B_COUNT=${B_COUNT:-0}
B_FINAL_MISS=$(echo "$B_MISSES" | tail -n1)
B_FINAL_MISS=${B_FINAL_MISS:-0}

if [ "$B_COUNT" -lt 5 ]; then
    echo "FAIL: expected >=5 PeriodicB releases in ~3s (80ms period), got $B_COUNT"
    FAIL=1
fi
if [ "$B_FINAL_MISS" -lt 2 ]; then
    echo "FAIL: expected PeriodicB's final miss count to be >=2 (every 3rd iteration deliberately overruns its deadline), got $B_FINAL_MISS"
    FAIL=1
fi

echo "PeriodicA: $A_COUNT releases, 0 misses (as expected)"
echo "PeriodicB: $B_COUNT releases, final miss count=$B_FINAL_MISS"

if [ "$FAIL" -eq 1 ]; then
    cat "$LOG"
    exit 1
fi

echo "PASS: RMS check reports SCHEDULABLE; PeriodicA never misses ($A_COUNT releases); PeriodicB accumulates real deadline misses ($B_FINAL_MISS) from its deliberate overruns"
exit 0
