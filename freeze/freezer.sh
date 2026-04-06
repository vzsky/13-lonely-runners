#!/usr/bin/env bash
set -euo pipefail

EXPECTED_FILE="./freeze/frozen"
TMP_OUT="/tmp/result"
SUMMARY="${TMP_OUT}.summary"

./run.sh main.cpp > "$TMP_OUT"
./log_summary.py "$TMP_OUT" > "$SUMMARY"

tail -n 3 $TMP_OUT

if diff -u "$EXPECTED_FILE" "$SUMMARY"; then
  echo "PASS: regression test matched expected result"
else
  echo "FAIL: regression test mismatch"
  exit 1
fi
