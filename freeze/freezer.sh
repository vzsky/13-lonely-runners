#!/usr/bin/env bash
set -euo pipefail

EXPECTED_FILE="./frozen"
TMP_OUT="/tmp/result"
SUMMARY="${TMP_OUT}.summary"

clang++ -std=c++23 -march=native -O3 freeze.cpp -o run_binary
./run_binary > "$TMP_OUT"
../log_summary.py "$TMP_OUT" > "$SUMMARY"

tail -n 3 $TMP_OUT

if diff -u "$EXPECTED_FILE" "$SUMMARY"; then
  echo "PASS: regression test matched expected result"
else
  echo "FAIL: regression test mismatch"
  exit 1
fi
