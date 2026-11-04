#!/usr/bin/env bash

if [[ $# -ne 1 ]]; then
    printf 'Usage: %s <TEST>\n tests: yuy2_from_bgrx | yuy2_to_bgra\n' "$0"
    exit 2;
fi

# Paths
DIR_SCRIPT="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
DIR_LOGS_ABS="$DIR_SCRIPT/logs"
DIR_LOGS="scripts/logs"
DIR_BIN="bin"

# Exit on error
set -e

# Number of CPUs
CPU_COUNT=$(getconf _NPROCESSORS_ONLN 2>/dev/null || \
            getconf NPROCESSORS_ONLN 2>/dev/null)
CPU_COUNT="${CPU_COUNT:-1}"
[[ ! "$CPU_COUNT" =~ ^[1-9][0-9]*$ ]] && CPU_COUNT=1

# Build
cd -- "$DIR_SCRIPT/.."
make clean >/dev/null 2>&1
make test_simd_full

# Create log directory
mkdir $DIR_LOGS >/dev/null 2>&1

# Track worker PIDs
declare -a pids=()

# Setup trap to handle SIGINT and SIGTERM
trap 'printf "\nAborted, cleaning up workers..."
      kill "${pids[@]}" >/dev/null 2>&1
      wait >/dev/null 2>&1
      echo finished
      exit 1' INT TERM

# Calculate block distribution
blocks=10000
quotient=$((blocks / CPU_COUNT))
remainder=$((blocks % CPU_COUNT))

# Start tests
echo "Starting test $1 on $CPU_COUNT CPUs"

start=$((remainder + quotient))
$DIR_BIN/test_simd_full "$1" 0 $start > $DIR_LOGS/0.log 2>&1 &
pids[0]=$!

for ((i = 1; i < CPU_COUNT; i++)); do
    $DIR_BIN/test_simd_full "$1" $start $quotient > $DIR_LOGS/$i.log 2>&1 &
    pids[$i]=$!
    start=$((start + quotient))
done

# Wait for tests
failed=0
for i in "${!pids[@]}"; do
    if ! wait "${pids[i]}"; then
        printf 'Error: Worker %d (PID %s) failed, see log: %s/%d.log\n' \
               $i ${pids[i]} "$DIR_LOGS_ABS" $i >&2
        failed=1
    fi
done

# Fail if any worker failed
[[ $failed -ne 0 ]] && exit 1

echo "Completed test $1 successfully"
