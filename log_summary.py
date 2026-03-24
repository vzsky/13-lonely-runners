#!/usr/bin/env python3
import re
import math
import sys
import os
from dataclasses import dataclass, field
from typing import Optional


def is_prime(n: int) -> bool:
    if n < 2:
        return False
    if n < 4:
        return True
    if n % 2 == 0 or n % 3 == 0:
        return False
    i = 5
    while i * i <= n:
        if n % i == 0 or n % (i + 2) == 0:
            return False
        i += 6
    return True


@dataclass
class PrimeRun:
    p: int
    k: int
    steps: list = field(default_factory=list)   # [(trying_n, t_size, result_n, s_size), ...]
    step1_s_size: Optional[int] = None           # S size from "Step1 (n=1): S size = X"
    final_s_zero: bool = False
    counter_example: bool = False
    counter_example_mod: Optional[int] = None    # the modulus from "Counter Example Mod N"
    counter_example_s_size: Optional[int] = None # S size at the point of counter example

    def resolvable(self) -> bool:
        """Counter example with S=k and k+1 prime — counts as valid."""
        return (
            self.counter_example
            and self.counter_example_s_size == self.k
            and is_prime(self.k + 1)
        )

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
            s_str = f" S={self.counter_example_s_size}" if self.counter_example_s_size is not None else ""
            if self.resolvable():
                lines.append(f"    ** Counter Example{s_str}  [resolvable: k+1={self.k+1} is prime => valid] **")
            else:
                lines.append(f"    ** Counter Example{s_str} **")
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

    pending_trying: Optional[tuple] = None  # (n, t_size)

    def flush_run():
        nonlocal pending_trying
        if current is not None:
            if pending_trying is not None:
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
                    current.steps.append((-1, -1, result_n, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                continue

            # ── "Counter Example Mod N" ──────────────────────────────────────
            m = re_counter.search(line)
            if m:
                current.counter_example = True
                current.counter_example_mod = int(m.group(1))
                # capture S size from the last recorded step
                if current.steps:
                    last_s = current.steps[-1][3]  # s_size is index 3
                    if last_s >= 0:
                        current.counter_example_s_size = last_s
                continue

    flush_run()
    if current is not None:
        runs.append(current)

    return runs


LOG_EXTENSIONS = {".log", ".txt", ""}   # "" = extensionless files

def collect_log_files(path: str) -> list[str]:
    """Return a sorted list of log file paths from a file or directory."""
    if os.path.isfile(path):
        return [path]
    if os.path.isdir(path):
        files = []
        for entry in sorted(os.scandir(path), key=lambda e: e.name):
            if not entry.is_file() or entry.name.startswith("."):
                continue
            ext = os.path.splitext(entry.name)[1].lower()
            if ext in LOG_EXTENSIONS:
                files.append(entry.path)
        if not files:
            print(f"No log files found in directory: {path}")
            print(f"  (looked for extensions: {sorted(LOG_EXTENSIONS)})")
            sys.exit(1)
        return files
    print(f"Error: '{path}' is neither a file nor a directory.")
    sys.exit(1)


def process_files(paths: list[str]) -> list[PrimeRun]:
    """Parse all log files and return merged runs + k."""
    all_runs: list[PrimeRun] = []
    for path in paths:
        runs = parse_log(path)
        if runs:
            all_runs.extend(runs)
        else:
            print(f"  (no parameter blocks found in {path})")
    return all_runs


def print_results(runs: list[PrimeRun], source_label: str):
    if not runs:
        print("No parameter blocks found.")
        return

    k = runs[0].k
    if any(r.k != k for r in runs):
        print("WARNING: mixed k values – using k from first block")

    # ── per-run detail ───────────────────────────────────────────────────────
    print("=" * 60)
    print(f"PER-PRIME DETAIL  [{source_label}]")
    print("=" * 60)
    for run in runs:
        print(run.summary())
        print()

    # ── proof check ─────────────────────────────────────────────────────────
    valid_primes = [r.p for r in runs if (r.final_s_zero and not r.counter_example) or r.resolvable()]
    ok, s, tres = is_primes_enough(valid_primes, k)

    n_valid      = sum(1 for r in runs if r.final_s_zero and not r.counter_example)
    n_resolvable = sum(1 for r in runs if r.resolvable())
    n_counter    = sum(1 for r in runs if r.counter_example and not r.resolvable())
    n_other      = sum(1 for r in runs if not r.final_s_zero and not r.counter_example)

    # Counter example details (unresolvable only)
    counter_runs = [r for r in runs if r.counter_example and not r.resolvable()]

    print("=" * 60)
    print(f"PROOF SUMMARY  [{source_label}]")
    print("=" * 60)
    print(f"k                = {k}")
    print(f"total runs       = {len(runs)}")
    print(f"  S=0 (valid)    = {n_valid}")
    print(f"  resolvable CE  = {n_resolvable}  (S=k and k+1={k+1} is prime)")
    print(f"  counter-example= {n_counter}")
    if counter_runs:
        for r in counter_runs:
            s_str = f"S={r.counter_example_s_size}" if r.counter_example_s_size is not None else "S=?"
            print(f"    p={r.p}  {s_str}")
    print(f"  other          = {n_other}")
    print(f"valid primes     = {sorted(valid_primes)}")
    print(f"log prod(primes) = {s:.6f}")
    print(f"log threshold    = {tres:.6f}")
    print(f"PROOF:             {'YES' if ok else 'NO'}")


def main():
    if len(sys.argv) != 2:
        print("usage: python check_proof.py <logfile|folder>")
        sys.exit(1)

    input_path = sys.argv[1]
    log_files = collect_log_files(input_path)

    if os.path.isdir(input_path):
        # ── folder mode: print per-file then combined ────────────────────────
        print(f"Found {len(log_files)} file(s) in '{input_path}'")
        print()

        all_runs: list[PrimeRun] = []
        for path in log_files:
            label = os.path.basename(path)
            try:
                runs = parse_log(path)
            except Exception as e:
                print(f"  (skipping {label}: {e})")
                continue
            if runs:
                print_results(runs, label)
                print()
                all_runs.extend(runs)
            else:
                print(f"  (no blocks in {label})")

        if len(log_files) > 1 and all_runs:
            print()
            print("*" * 60)
            print("COMBINED RESULTS")
            print("*" * 60)
            print_results(all_runs, "ALL FILES")
    else:
        # ── single file mode ─────────────────────────────────────────────────
        runs = parse_log(input_path)
        print_results(runs, os.path.basename(input_path))


if __name__ == "__main__":
    main()
