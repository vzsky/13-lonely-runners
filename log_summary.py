#!/usr/bin/env python3
import re
import math
import sys
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class PrimeRun:
    p: int
    k: int
    steps: list = field(default_factory=list)   # [(trying_n, t_size, result_n, s_size), ...]
    step1_s_size: Optional[int] = None           # S size from "Step1 (n=1): S size = X"
    final_s_zero: bool = False
    counter_example: bool = False

    def summary(self) -> str:
        lines = [f"  p={self.p}  k={self.k}"]
        prv_s = self.step1_s_size
        for trying_n, t_size, result_n, s_size in self.steps:
            prev_str = f"Prev S={prv_s:>8}" if prv_s is not None else "Prev S=       ?"
            lines.append(
                f"    {prev_str}  trying n={trying_n:>3}  T={t_size:>8}  =>  n={result_n:>3}  S={s_size:>8}"
            )
            prv_s = s_size
        if self.counter_example:
            lines.append("    ** Counter Example **")
        elif self.final_s_zero:
            lines.append("    => S=0  (success)")
        else:
            lines.append("    => (no S=0 seen)")
        return "\n".join(lines)


def is_primes_enough(primes, k):
    uniq_primes = sorted(set(primes))
    if len(uniq_primes) != len(primes):
        print("WARNING: Prime in list is not unique")
    s = sum(math.log(p) for p in uniq_primes)
    binom = k * (k + 1) / 2.0
    base = (binom ** (k - 1)) / k
    tres = k * math.log(base)
    return s > tres, s, tres


def parse_log(path):
    runs: list[PrimeRun] = []
    current: Optional[PrimeRun] = None

    re_params   = re.compile(r"Parameters:\s*p\s*=\s*(\d+),\s*k\s*=\s*(\d+)")
    re_step1    = re.compile(r"Step1\s+\(n=1\):\s*S\s+size\s*=\s*(\d+)")
    re_trying   = re.compile(r"trying\s+(\d+):\s*T\s+size\s*=\s*(\d+)")
    re_result   = re.compile(r"=>\s*\(n=(\d+)\):\s*S\s+size\s*=\s*(\d+)")
    re_counter  = re.compile(r"Counter\s+Example\s+Mod\s+(\d+)")

    # We accumulate trying/result pairs in a small buffer because they come in pairs
    pending_trying: Optional[tuple] = None  # (n, t_size)

    def flush_run():
        nonlocal pending_trying
        if current is not None:
            if pending_trying is not None:
                # trying line with no matching result – store with result=-1/-1
                current.steps.append((*pending_trying, -1, -1))
                pending_trying = None

    with open(path, "r") as f:
        for line in f:
            # ── new prime block ──────────────────────────────────────────────
            m = re_params.search(line)
            if m:
                flush_run()
                if current is not None:
                    runs.append(current)
                current = PrimeRun(p=int(m.group(1)), k=int(m.group(2)))
                pending_trying = None
                continue

            if current is None:
                continue

            # ── "Step1 (n=1): S size = X" ────────────────────────────────────
            m = re_step1.search(line)
            if m:
                current.step1_s_size = int(m.group(1))
                continue

            # ── "trying N: T size = X" ───────────────────────────────────────
            m = re_trying.search(line)
            if m:
                if pending_trying is not None:
                    # previous trying had no result line – store as-is
                    current.steps.append((*pending_trying, -1, -1))
                pending_trying = (int(m.group(1)), int(m.group(2)))
                continue

            # ── "=> (n=N): S size = X" ───────────────────────────────────────
            m = re_result.search(line)
            if m:
                result_n  = int(m.group(1))
                s_size    = int(m.group(2))
                if pending_trying is not None:
                    current.steps.append((*pending_trying, result_n, s_size))
                    pending_trying = None
                else:
                    # result without a preceding trying line
                    current.steps.append((-1, -1, result_n, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                continue

            # ── "Counter Example Mod N" ──────────────────────────────────────
            m = re_counter.search(line)
            if m:
                current.counter_example = True
                continue

    # flush last block
    flush_run()
    if current is not None:
        runs.append(current)

    return runs


def main():
    if len(sys.argv) != 2:
        print("usage: python check_proof.py <logfile>")
        sys.exit(1)

    runs = parse_log(sys.argv[1])
    if not runs:
        print("No parameter blocks found in log.")
        sys.exit(1)

    k = runs[0].k
    if any(r.k != k for r in runs):
        print("WARNING: mixed k values in log – using k from first block")

    # ── per-run detail ───────────────────────────────────────────────────────
    print("=" * 60)
    print("PER-PRIME DETAIL")
    print("=" * 60)
    for run in runs:
        print(run.summary())
        print()

    # ── proof check ─────────────────────────────────────────────────────────
    valid_primes = [r.p for r in runs if r.final_s_zero and not r.counter_example]
    ok, s, tres = is_primes_enough(valid_primes, k)

    print("=" * 60)
    print("PROOF SUMMARY")
    print("=" * 60)
    print(f"k                = {k}")
    print(f"total runs       = {len(runs)}")
    print(f"  S=0 (valid)    = {sum(1 for r in runs if r.final_s_zero and not r.counter_example)}")
    print(f"  counter-example= {sum(1 for r in runs if r.counter_example)}")
    print(f"  other          = {sum(1 for r in runs if not r.final_s_zero and not r.counter_example)}")
    print(f"valid primes     = {sorted(valid_primes)}")
    print(f"log prod(primes) = {s:.6f}")
    print(f"log threshold    = {tres:.6f}")
    print(f"PROOF:             {'YES' if ok else 'NO'}")


if __name__ == "__main__":
    main()
