#!/usr/bin/env bash
set -uo pipefail

ELF="rtos/build/m0.elf"
LOG="rtos/build/m0_output.log"
EXPECTED="M0: task0 alive"

mkdir -p rtos/build
qemu-system-arm -M mps2-an385 -kernel "$ELF" -nographic > "$LOG" 2>&1 &
QEMU_PID=$!
sleep 3
kill "$QEMU_PID" 2>/dev/null
wait "$QEMU_PID" 2>/dev/null

COUNT=$(grep -c "$EXPECTED" "$LOG" 2>/dev/null || echo 0)

if [ "$COUNT" -ge 2 ]; then
    echo "PASS: saw '$EXPECTED' $COUNT times"
    exit 0
else
    echo "FAIL: expected >=2 occurrences of '$EXPECTED', got $COUNT"
    cat "$LOG"
    exit 1
fi
