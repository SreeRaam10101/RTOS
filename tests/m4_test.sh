#!/usr/bin/env bash
set -uo pipefail

run_and_capture() {
    local elf="$1"
    local log="$2"
    qemu-system-arm -M mps2-an385 -kernel "$elf" -nographic > "$log" 2>&1 &
    local pid=$!
    sleep 3
    kill "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
}

mkdir -p rtos/build

run_and_capture rtos/build/kernel_no_inherit.elf rtos/build/m4_no_inherit.log
run_and_capture rtos/build/kernel.elf rtos/build/m4_inherit.log

NO_INHERIT_LOG=$(tr -d '\r' < rtos/build/m4_no_inherit.log 2>/dev/null)
INHERIT_LOG=$(tr -d '\r' < rtos/build/m4_inherit.log 2>/dev/null)

NO_INHERIT_HIGH=$(echo "$NO_INHERIT_LOG" | grep -c '^High: acquired')
NO_INHERIT_HIGH=${NO_INHERIT_HIGH:-0}
NO_INHERIT_MEDIUM=$(echo "$NO_INHERIT_LOG" | grep -c '^Medium: running')
NO_INHERIT_MEDIUM=${NO_INHERIT_MEDIUM:-0}

INHERIT_HIGH=$(echo "$INHERIT_LOG" | grep -c '^High: acquired')
INHERIT_HIGH=${INHERIT_HIGH:-0}

echo "no-inheritance build: High acquired=$NO_INHERIT_HIGH, Medium running=$NO_INHERIT_MEDIUM"
echo "with-inheritance build: High acquired=$INHERIT_HIGH"

FAIL=0

if [ "$NO_INHERIT_MEDIUM" -lt 20 ]; then
    echo "FAIL: expected Medium to run many times in the no-inheritance build (proving it's actually hogging the CPU), got $NO_INHERIT_MEDIUM"
    FAIL=1
fi

if [ "$NO_INHERIT_HIGH" -ne 0 ]; then
    echo "FAIL: expected High to be fully starved (0 acquisitions) in the no-inheritance build, got $NO_INHERIT_HIGH -- inversion not demonstrated"
    FAIL=1
fi

if [ "$INHERIT_HIGH" -lt 1 ]; then
    echo "FAIL: expected High to acquire the mutex at least once in the with-inheritance build, got $INHERIT_HIGH -- inheritance fix not working"
    FAIL=1
fi

if [ "$FAIL" -eq 1 ]; then
    echo "--- no-inheritance log ---"
    cat rtos/build/m4_no_inherit.log
    echo "--- with-inheritance log ---"
    cat rtos/build/m4_inherit.log
    exit 1
fi

echo "PASS: no-inheritance build starves High (0 acquisitions) while Medium hogs the CPU ($NO_INHERIT_MEDIUM runs); with-inheritance build lets High acquire the mutex ($INHERIT_HIGH times) -- priority inversion demonstrated and fixed"
exit 0
