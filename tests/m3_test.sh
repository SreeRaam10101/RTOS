#!/usr/bin/env bash
set -uo pipefail

ELF="rtos/build/kernel.elf"
LOG="rtos/build/m3_output.log"

mkdir -p rtos/build
qemu-system-arm -M mps2-an385 -kernel "$ELF" -nographic > "$LOG" 2>&1 &
QEMU_PID=$!
sleep 3
kill "$QEMU_PID" 2>/dev/null
wait "$QEMU_PID" 2>/dev/null

CLEAN_LOG=$(tr -d '\r' < "$LOG" 2>/dev/null)

VALUES=$(echo "$CLEAN_LOG" | grep '^consumer: shared_counter=' | sed 's/^consumer: shared_counter=//')
COUNT=$(echo "$VALUES" | grep -c '^[0-9]')
COUNT=${COUNT:-0}

echo "consumer print count: $COUNT"

if [ "$COUNT" -lt 10 ]; then
    echo "FAIL: expected >=10 consumer prints in ~3s (100ms producer period), got $COUNT"
    cat "$LOG"
    exit 1
fi

if [ "$COUNT" -gt 60 ]; then
    echo "FAIL: got $COUNT consumer prints, far more than the ~30 expected from a 100ms period over 3s -- delay() may not be pacing correctly"
    cat "$LOG"
    exit 1
fi

EXPECTED=1
FAIL=0
while IFS= read -r v; do
    if [ "$v" != "$EXPECTED" ]; then
        echo "FAIL: expected shared_counter=$EXPECTED next, got $v -- sequence not strictly sequential (mutex/semaphore bug)"
        FAIL=1
        break
    fi
    EXPECTED=$((EXPECTED + 1))
done <<< "$VALUES"

if [ "$FAIL" -eq 1 ]; then
    cat "$LOG"
    exit 1
fi

echo "PASS: $COUNT consumer prints, strictly sequential 1..$COUNT (delay+semaphore+mutex all correct)"
exit 0
