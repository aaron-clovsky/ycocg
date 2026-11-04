#!/usr/bin/env bash

# Paths
DIR_BIN=bin
DIR_OUTPUT=$DIR_BIN/output

# Return value
retval=0

# Required commands
if ! command -v grep >/dev/null 2>&1; then
  echo "grep is not installed, please install it"
  exit 2
fi

if ! command -v cc >/dev/null 2>&1; then
  echo "C compiler not installed, please install one"
  exit 2
fi

if ! command -v verilator >/dev/null 2>&1; then
  echo "verilator is not installed, please install verilator"
  exit 2
fi

# Exit on error
set -e

# Build
SCRIPT_DIR="$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
cd -- "$SCRIPT_DIR/.."
make clean >/dev/null 2>&1
mkdir $DIR_OUTPUT >/dev/null 2>&1
make

# Run C reference
./$DIR_BIN/ycocg_tool allycocg > $DIR_OUTPUT/test_c.txt

# Run Verilog testbench
./$DIR_BIN/tb_ycocg_to_rgb_v +verilator+quiet | grep -v "Verilog \$finish" \
 > $DIR_OUTPUT/tb_v.txt

# Check
if diff $DIR_OUTPUT/tb_v.txt $DIR_OUTPUT/test_c.txt > /dev/null; then
  echo "Verilog Matches C"
  rm -f $DIR_OUTPUT/tb_v.txt
else
  echo "!!! Verilog DOES NOT MATCH C !!!"
  retval=1
fi

# Run SystemVerilog testbench
./$DIR_BIN/tb_ycocg_to_rgb_sv +verilator+quiet | grep -v "Verilog \$finish" \
 > $DIR_OUTPUT/tb_sv.txt

# Check
if diff $DIR_OUTPUT/tb_sv.txt $DIR_OUTPUT/test_c.txt > /dev/null; then
  echo "System Verilog Matches C"
  rm -f $DIR_OUTPUT/tb_sv.txt
else
  echo "!!! System Verilog DOES NOT MATCH C !!!"
  retval=1
fi

# Cleanup
if [ $retval -eq 0 ]; then
  rm -rf $DIR_OUTPUT
fi

exit $retval
