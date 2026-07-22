#!/usr/bin/env bash
set -uo pipefail

cd "$(dirname "$0")/../rtos"

FAIL=0

make clean >/dev/null
make >/dev/null

qemu-system-arm -M mps2-an385 -kernel build/kernel.elf -nographic > build/m7.log 2>&1 &
PID=$!
sleep 3
kill "$PID" 2>/dev/null
wait "$PID" 2>/dev/null

LOG=$(tr -d '\r' < build/m7.log)

REPORT_COUNT=$(echo "$LOG" | grep -c '^Report: avg=')
echo "Report lines captured: $REPORT_COUNT"
if [ "$REPORT_COUNT" -lt 16 ]; then
    echo "FAIL: expected at least 16 Report lines in the 3s capture window, got $REPORT_COUNT"
    FAIL=1
fi

# Steady-state avg cycle from the second 8-sample cycle onward (see
# rtos/main.c's moving-average derivation comment). Positions 3-10 (1-indexed)
# skip the 2-sample warmup where the zero-initialized history ring hasn't
# filled yet.
EXPECTED_CYCLE=(21 22 30 38 38 30 21 20)
ACTUAL_AVGS=$(echo "$LOG" | grep '^Report: avg=' | sed 's/^Report: avg=\([0-9]*\).*/\1/')
CHECK_AVGS=$(echo "$ACTUAL_AVGS" | sed -n '3,10p')

i=0
while IFS= read -r val; do
    if [ "$val" != "${EXPECTED_CYCLE[$i]}" ]; then
        echo "FAIL: steady-state avg mismatch at position $i: expected ${EXPECTED_CYCLE[$i]}, got $val"
        FAIL=1
    fi
    i=$((i + 1))
done <<< "$CHECK_AVGS"

ALERT_COUNT=$(echo "$LOG" | grep -c '^ALERT:')
echo "ALERT lines: $ALERT_COUNT"
if [ "$ALERT_COUNT" -lt 2 ]; then
    echo "FAIL: expected at least 2 ALERT lines (one full 8-sample cycle produces exactly 2), got $ALERT_COUNT"
    FAIL=1
fi

BAD_LATENCY=$(echo "$LOG" | grep '^ALERT:' | sed 's/.*detect_tick=\([0-9]*\) gpio_tick=\([0-9]*\).*/\1 \2/' | awk '{d=$2-$1; if (d<0 || d>2) print}')
if [ -n "$BAD_LATENCY" ]; then
    echo "FAIL: found ALERT line(s) with gpio_tick-detect_tick outside [0,2]: $BAD_LATENCY"
    FAIL=1
fi

LAST_REPORT=$(echo "$LOG" | grep '^Report:' | tail -1)
LAST_OVERFLOW_SAMPLE=$(echo "$LAST_REPORT" | sed 's/.*overflow_sample=\([0-9]*\).*/\1/')
LAST_OVERFLOW_FILTER=$(echo "$LAST_REPORT" | sed 's/.*overflow_filter=\([0-9]*\).*/\1/')
LAST_OVERFLOW_REPORT=$(echo "$LAST_REPORT" | sed 's/.*overflow_report=\([0-9]*\).*/\1/')

echo "Final overflow counts: sample=$LAST_OVERFLOW_SAMPLE filter=$LAST_OVERFLOW_FILTER report=$LAST_OVERFLOW_REPORT"
if [ "$LAST_OVERFLOW_SAMPLE" != "0" ] || [ "$LAST_OVERFLOW_FILTER" != "0" ] || [ "$LAST_OVERFLOW_REPORT" != "0" ]; then
    echo "FAIL: expected all queue overflow counts to be 0"
    FAIL=1
fi

make clean >/dev/null

if [ "$FAIL" -eq 1 ]; then
    echo "FAIL: one or more M7 pipeline assertions did not match expected behavior"
    exit 1
fi

echo "PASS: interrupt-driven ADC pipeline produced expected steady-state averages, bounded-latency alerts, and zero queue overflows"
exit 0
