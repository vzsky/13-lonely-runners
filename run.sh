#!/usr/bin/env bash

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 file"
  exit 1
fi

SRC=$1
OPTIM="-O3"

for arg in "$@"; do
  if [ "$arg" = "--debug" ]; then
    OPTIM="-O0"
    break
  fi
done

now_ns() {
  date +%s%N
}

ns_to_s() {
  awk "BEGIN { printf \"%.6f\", $1 / 1e9 }"
}

compile_start=$(now_ns)
clang++ -std=c++23 -march=native "$OPTIM" "$SRC" -o run_binary
compile_status=$?
compile_end=$(now_ns)

if [ $compile_status -ne 0 ]; then
  echo "Compilation failed"
  exit 1
fi

compile_elapsed_ns=$((compile_end - compile_start))
echo "Compilation time: $(ns_to_s "$compile_elapsed_ns") s"
echo "Execution start: $(date -Iseconds)"

run_start=$(now_ns)
./run_binary
run_end=$(now_ns)

run_elapsed_ns=$((run_end - run_start))

echo "Compilation time: $(ns_to_s "$compile_elapsed_ns") s"
echo "Execution time: $(ns_to_s "$run_elapsed_ns") s"
