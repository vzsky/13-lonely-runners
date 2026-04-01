#!/usr/bin/env python3
import re
import math
import sys
import os
from dataclasses import dataclass, field
from typing import Optional
from fractions import Fraction

# ══════════════════════════════════════════════════════════════════════════════
# DATA
# ══════════════════════════════════════════════════════════════════════════════

@dataclass
class PrimeRun:
    p: int
    k: int
    steps: list = field(default_factory=list)       # [(trying_n, t_size, result_n, s_size), ...]
    step1_s_size: Optional[int] = None
    final_s_zero: bool = False
    counter_example: bool = False
    counter_example_mod: Optional[int] = None
    counter_example_s_size: Optional[int] = None
    seeds: list = field(default_factory=list)       # frozensets


# ══════════════════════════════════════════════════════════════════════════════
# PARSING
# ══════════════════════════════════════════════════════════════════════════════

LOG_EXTENSIONS = {".log", ".txt", ""}


def collect_log_files(path: str) -> list[str]:
    if os.path.isfile(path):
        return [path]
    if os.path.isdir(path):
        files = [
            e.path
            for e in sorted(os.scandir(path), key=lambda e: e.name)
            if e.is_file()
            and not e.name.startswith(".")
            and os.path.splitext(e.name)[1].lower() in LOG_EXTENSIONS
        ]
        if not files:
            print(f"No log files found in directory: {path}")
            print(f"  (looked for extensions: {sorted(LOG_EXTENSIONS)})")
            sys.exit(1)
        return files
    print(f"Error: '{path}' is neither a file nor a directory.")
    sys.exit(1)


def parse_log(path: str) -> list[PrimeRun]:
    runs: list[PrimeRun] = []
    current: Optional[PrimeRun] = None
    pending_trying: Optional[tuple] = None

    re_params = re.compile(r"Parameters:\s*p\s*=\s*(\d+),\s*k\s*=\s*(\d+)")
    re_step1 = re.compile(r"Step1\s+\(n=1\):\s*S\s+size\s*=\s*(\d+)")
    re_trying = re.compile(r"trying\s+(\d+):\s*T\s+size\s*=\s*(\d+)")
    re_result = re.compile(r"=>\s*\(n=(\d+)\):\s*S\s+size\s*=\s*(\d+)")
    re_trying_inline = re.compile(
        r"trying\s+(\d+):\s*T\s+size\s*=\s*(\d+)\s*=>\s*\(n=(\d+)\):\s*S\s+size\s*=\s*(\d+)"
    )
    re_counter = re.compile(r"Counter\s+Example\s+Mod\s+(\d+)")
    re_seeds = re.compile(r"seeds\s*:(.*)")

    def flush():
        nonlocal pending_trying
        if current is not None and pending_trying is not None:
            current.steps.append((*pending_trying, -1, -1))
            pending_trying = None

    with open(path) as f:
        for line in f:
            if m := re_params.search(line):
                flush()
                if current is not None:
                    runs.append(current)
                current = PrimeRun(p=int(m.group(1)), k=int(m.group(2)))
                pending_trying = None
                continue

            if current is None:
                continue

            if m := re_step1.search(line):
                current.step1_s_size = int(m.group(1))
                continue

            if m := re_trying_inline.search(line):
                trying_n = int(m.group(1))
                t_size = int(m.group(2))
                result_n = int(m.group(3))
                s_size = int(m.group(4))
                current.steps.append((trying_n, t_size, result_n, s_size))
                pending_trying = None
                if s_size == 0:
                    current.final_s_zero = True
                continue

            if m := re_trying.search(line):
                if pending_trying is not None:
                    current.steps.append((*pending_trying, -1, -1))
                pending_trying = (int(m.group(1)), int(m.group(2)))
                continue

            if m := re_result.search(line):
                result_n = int(m.group(1))
                s_size = int(m.group(2))
                if pending_trying is not None:
                    current.steps.append((*pending_trying, result_n, s_size))
                    pending_trying = None
                else:
                    current.steps.append((-1, -1, result_n, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                continue

            if m := re_seeds.search(line):
                current.seeds = [
                    frozenset(int(x) for x in g.split())
                    for g in re.findall(r"\(([^)]+)\)", m.group(1))
                ]
                continue

            if m := re_counter.search(line):
                current.counter_example = True
                current.counter_example_mod = int(m.group(1))
                if current.steps and current.steps[-1][3] >= 0:
                    current.counter_example_s_size = current.steps[-1][3]
                continue

    flush()
    if current is not None:
        runs.append(current)

    return runs


def parse_all(paths: list[str]) -> list[PrimeRun]:
    all_runs = []
    for path in paths:
        runs = parse_log(path)
        if runs:
            all_runs.extend(runs)
        else:
            print(f"  (no parameter blocks found in {path})")
    return all_runs


# ══════════════════════════════════════════════════════════════════════════════
# VERIFYING
# ══════════════════════════════════════════════════════════════════════════════

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


def check_seed(seed, p, k) -> tuple[bool, Optional[int], dict]:
    target = set(range(1, k + 1))
    for a in range(1, p):
        image = set((a * s) % p for s in seed)
        if len(image) != k:
            continue
        abs_image = set(min(x, p - x) for x in image)
        if abs_image == target:
            signs = {min(x, p - x): ('+' if x <= p // 2 else '-') for x in image}
            return True, a, signs
    return False, None, {}

def fractional_part(x: Fraction) -> Fraction:
    return x - math.floor(x)

def r_vector(t: Fraction, n: int, k: int) -> tuple:
    return tuple(
        math.floor(n * fractional_part(j * t))
        for j in range(1, k + 1)
    )

def all_r_vectors_rational(n: int, k: int, p: int) -> list[tuple]:
    # Find all possible r(x) = (R_n(x), R_n(2x), ..., R_n(kx))
    seen = set()

    for m in range(p):
        x = Fraction(m, p)
        vec = r_vector(x, n, k)
        if vec not in seen:
            seen.add(vec)

    return list(seen)

def valid_resolve_modulo (p, k) -> bool: 
    return set(all_r_vectors_rational(k+1, k, k+1)).issubset(set(all_r_vectors_rational(k+1, k, p)))

def is_primes_enough(primes: list[int], k: int) -> tuple[bool, float, float]:
    uniq = sorted(set(primes))
    if len(uniq) != len(primes):
        print("WARNING: Prime list contains duplicates")
    s = sum(math.log(p) for p in uniq)
    binom = k * (k + 1) / 2.0
    threshold = k * math.log((binom ** (k - 1)) / k)
    return s > threshold, s, threshold


# ══════════════════════════════════════════════════════════════════════════════
# WRITING
# ══════════════════════════════════════════════════════════════════════════════

def format_run(run: PrimeRun) -> tuple[bool, str]:
    """Return (is_valid, formatted_string) for a single run."""
    lines = [f"  p={run.p}  k={run.k}"]
    prev_s = run.step1_s_size
    for trying_n, t_size, result_n, s_size in run.steps:
        prev_str = f"Prev S={prev_s:>8}" if prev_s is not None else "Prev S=       ?"
        lines.append(
            f"    {prev_str}  trying n={trying_n:>3}  T={t_size:>8}  =>  n={result_n:>3}  S={s_size:>8}"
        )
        prev_s = s_size

    if prev_s == 0: 
        lines.append("    => S=0  (success)")
        return True, "\n".join(lines)

    if run.counter_example:
        s_str = f" S={run.counter_example_s_size}" if run.counter_example_s_size is not None else ""
        if run.counter_example_s_size is not None and len(run.seeds) != run.counter_example_s_size:
            lines.append(f"    ERROR: seed size {len(run.seeds)} != cx size {run.counter_example_s_size}")
            return False, "\n".join(lines)
        if not is_prime(run.k + 1):
            lines.append(f"    Cannot resolve remaining seed for non prime k+1")
            return False, "\n".join(lines)
        if not valid_resolve_modulo (run.p, run.k): 
            lines.append(f"    Given {run.p} is not a valid candidate for resolve (r(k+1) is not subset of r(p))")
            return False, "\n".join(lines)
        seed_results = [(seed, *check_seed(seed, run.p, run.k)) for seed in run.seeds]
        valid = all(ok for _, ok, _, _ in seed_results)
        lines.append(f"    ** REMAINING SEEDS {s_str}  [{'resolvable' if valid else 'UNRESOLVABLE'}] **")
        for i, (seed, ok, a, _) in enumerate(seed_results):
            seed_str = "{" + ", ".join(str(x) for x in sorted(seed)) + "}"
            lines.append(f"      seed[{i}] {seed_str}  =>  {'PASS' if ok else 'FAIL'}" + (f"  (a={a})" if ok else ""))
        return valid, "\n".join(lines);

    if run.final_s_zero:
        lines.append("    => S=0  (success)")
        return True, "\n".join(lines)

    return False, "\n".join(lines + ["    END OF BLOCK - UNKNOWN RESULT"])


def print_results(runs: list[PrimeRun], source_label: str, detail: bool = True):
    if not runs:
        print("No parameter blocks found.")
        return

    k = runs[0].k
    if any(r.k != k for r in runs):
        print("FATAL: mixed k values")
        exit(0)

    formatted = [format_run(run) for run in runs]

    if detail:
        print("=" * 60)
        print(f"PER-PRIME DETAIL  [{source_label}]")
        print("=" * 60)
        for _, text in formatted:
            print(text)
            print()

    pairs = list(zip(runs, formatted))
    valid_primes   = [run.p for run, (valid, _) in pairs if valid]
    invalid_primes = [run.p for run, (valid, _) in pairs if not valid]
    ok, s, thresh  = is_primes_enough(valid_primes, k)

    unique_valid   = sorted(set(valid_primes))
    unique_invalid = sorted(set(invalid_primes))

    print("=" * 60)
    print(f"PROOF SUMMARY  [{source_label}]")
    print("=" * 60)
    print(f"k                       = {k}")
    print(f"total runs              = {len(runs)}")
    print(f"valid count / unique count            = {len(valid_primes)} / {len(unique_valid)}")
    print(f"invalid count / unique count          = {len(invalid_primes)} / {len(unique_invalid)}")
    print(f"unique valid primes            = {unique_valid}")
    print(f"unique invalid primes          = {unique_invalid}")
    print(f"log prod(valid primes)  = {s:.6f}")
    print(f"log threshold           = {thresh:.6f}")
    print(f"PROOF:                  {'YES' if ok else 'NO'}")

# ══════════════════════════════════════════════════════════════════════════════
# MAIN
# ══════════════════════════════════════════════════════════════════════════════

def main():
    if len(sys.argv) != 2:
        print(f"usage: python {os.path.basename(__file__)} <logfile|folder>")
        sys.exit(1)

    input_path = sys.argv[1]
    log_files  = collect_log_files(input_path)

    if os.path.isdir(input_path):
        print(f"Found {len(log_files)} file(s) in '{input_path}'\n")
        all_runs = []
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
            print("\n" + "*" * 60)
            print("COMBINED RESULTS")
            print("*" * 60)
            print_results(all_runs, "ALL FILES", detail=False)
    else:
        print_results(parse_log(input_path), os.path.basename(input_path))

if __name__ == "__main__":
    main()
