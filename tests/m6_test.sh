#!/usr/bin/env bash
set -uo pipefail

cd "$(dirname "$0")/../rtos"

FAIL=0

run_combo() {
    local sched=$1
    local workload=$2
    local expect_verdict=$3        # SCHEDULABLE or NOT SCHEDULABLE
    local expect_a_misses=$4       # "zero" or "any" (TaskA never misses in this demo)
    local expect_b_misses=$5       # "zero", "nonzero", or "either" (workload 4 EDF: don't pin which task)

    make clean >/dev/null
    make SCHED="$sched" WORKLOAD="$workload" >/dev/null
    local log="build/combo_${sched}_${workload}.log"
    qemu-system-arm -M mps2-an385 -kernel build/kernel.elf -nographic > "$log" 2>&1 &
    local pid=$!
    sleep 3
    kill "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null

    local clean_log
    clean_log=$(tr -d '\r' < "$log")

    local check_prefix="RMS check:"
    [ "$sched" = "EDF" ] && check_prefix="EDF check:"

    local check_line
    check_line=$(echo "$clean_log" | grep "^${check_prefix}")
    echo "[$sched/W$workload] $check_line"

    if ! echo "$check_line" | grep -q "=> ${expect_verdict}\$"; then
        echo "FAIL [$sched/W$workload]: expected '=> ${expect_verdict}', got: $check_line"
        FAIL=1
    fi

    local a_misses b_misses
    a_misses=$(echo "$clean_log" | grep '^PeriodicA:' | sed 's/.*misses=//')
    b_misses=$(echo "$clean_log" | grep '^PeriodicB:' | sed 's/.*misses=//')
    local a_final b_final
    a_final=$(echo "$a_misses" | tail -n1); a_final=${a_final:-0}
    b_final=$(echo "$b_misses" | tail -n1); b_final=${b_final:-0}

    if [ "$expect_a_misses" = "zero" ] && [ "$a_final" -ne 0 ]; then
        echo "FAIL [$sched/W$workload]: expected TaskA (PeriodicA) final miss count 0, got $a_final"
        FAIL=1
    fi

    case "$expect_b_misses" in
        zero)
            if [ "$b_final" -ne 0 ]; then
                echo "FAIL [$sched/W$workload]: expected TaskB (PeriodicB) final miss count 0, got $b_final"
                FAIL=1
            fi
            ;;
        nonzero)
            if [ "$b_final" -eq 0 ]; then
                echo "FAIL [$sched/W$workload]: expected TaskB (PeriodicB) final miss count > 0, got $b_final"
                FAIL=1
            fi
            ;;
        either)
            local combined=$((a_final + b_final))
            if [ "$combined" -eq 0 ]; then
                echo "FAIL [$sched/W$workload]: expected combined TaskA+TaskB misses > 0 under overload, got 0"
                FAIL=1
            fi
            ;;
    esac

    echo "  TaskA final misses=$a_final, TaskB final misses=$b_final"
}

# RMS: workload 1 schedulable+clean; 2-4 the static bound is exceeded and TaskB genuinely misses.
run_combo RMS 1 "SCHEDULABLE"     zero zero
run_combo RMS 2 "NOT SCHEDULABLE" zero nonzero
run_combo RMS 3 "NOT SCHEDULABLE" zero nonzero
run_combo RMS 4 "NOT SCHEDULABLE" zero nonzero

# EDF: workloads 1-3 stay under U<=1 and never miss; workload 4 (U>1) misses are unavoidable for any scheduler.
run_combo EDF 1 "SCHEDULABLE"     zero zero
run_combo EDF 2 "SCHEDULABLE"     zero zero
run_combo EDF 3 "SCHEDULABLE"     zero zero
run_combo EDF 4 "NOT SCHEDULABLE" any either

make clean >/dev/null

if [ "$FAIL" -eq 1 ]; then
    echo "FAIL: one or more SCHED x WORKLOAD combinations did not match expectations"
    exit 1
fi

echo "PASS: all 8 SCHED x WORKLOAD combinations matched expected RMS/EDF schedulability verdicts and runtime miss behavior"
exit 0
