#!/usr/bin/env bash

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 file"
  exit 1
fi

SRC=$1
BIN=run_binary
PROFILE=cpu.prof
GP=$(brew --prefix gperftools 2>/dev/null)

if [ ! -d "$GP" ]; then
  echo "gperftools not found. Install with: brew install gperftools"
  exit 1
fi

now_ns() { date +%s%N; }
ns_to_s() { awk "BEGIN { printf \"%.6f\", $1 / 1e9 }"; }

compile_start=$(now_ns)

clang++ -std=c++23 -O3 -g \
  -I"$GP/include" \
  -L"$GP/lib" \
  -lprofiler \
  "$SRC" -o "$BIN"

compile_status=$?
compile_end=$(now_ns)

[ $compile_status -ne 0 ] && exit 1

echo "Compilation time: $(ns_to_s $((compile_end-compile_start))) s"

export CPUPROFILE=$PROFILE
export CPUPROFILE_FREQUENCY=1000

run_start=$(now_ns)
./"$BIN"
run_end=$(now_ns)

unset CPUPROFILE CPUPROFILE_FREQUENCY

echo "Compilation time: $(ns_to_s $((compile_end-compile_start))) s"
echo "Execution time: $(ns_to_s $((run_end-run_start))) s"
echo "Profile written to $PROFILE"
echo "View with: pprof ./$BIN $PROFILE"
