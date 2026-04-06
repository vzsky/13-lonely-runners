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
    steps: list = field(default_factory=list)
    step1_s_size: Optional[int] = None
    final_s_zero: bool = False
    counter_example: bool = False
    counter_example_mod: Optional[int] = None
    counter_example_s_size: Optional[int] = None
    seeds: list = field(default_factory=list)

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
            sys.exit(1)
        return files
    print(f"Error: '{path}' is neither a file nor a directory.")
    sys.exit(1)

def parse_log(path: str) -> list[PrimeRun]:
    runs: list[PrimeRun] = []
    current: Optional[PrimeRun] = None
    pending_trying: Optional[tuple] = None

    re_thread = re.compile(r"^\[THREAD\s+\d+\]\s*(.*)$")

    re_params = re.compile(r"Parameters:\s*p\s*=\s*(\d+),\s*k\s*=\s*(\d+)")
    re_trying = re.compile(r"trying\s+(\d+):\s*T\s+size\s*=\s*(\d+)")
    re_trying_inline = re.compile(
        r"trying\s+(\d+):\s*T\s+size\s*=\s*(\d+)\s*=>\s*\([nl]=(\d+)\):\s*S\s+size\s*=\s*(\d+)"
    )
    re_forcing = re.compile(r"Forcing\s+Lift\s+c=(\d+):\s*T\s+size\s*=\s*(\d+)")

    re_step_generic = re.compile(
        r"Step\s+[\d.]+\s+\([nl]=(\d+)\):\s*S\s+size\s*=\s*(\d+)"
    )

    re_result_simple = re.compile(
        r"=>\s*\([nl]=(\d+)\):\s*S\s+size\s*=\s*(\d+)"
    )

    re_result_squeeze = re.compile(
        r"=>\s*squeezing\s*\([nl]=(\d+)\):\s*S\s+size\s*=\s*(\d+)"
    )

    re_strategy_size = re.compile(
        r"(?:Squeeze|Force\s+\d+|Resolve\s+\w+|Print(?:\s+\d+)?)\s+S\.size\(\)\s*=\s*(\d+)"
    )

    re_counter = re.compile(r"Counter\s+Example\s+Mod\s+(\d+)")
    re_seeds = re.compile(r"seeds\s*:(.*)")

    re_step_plain = re.compile(r"Step\s+([\d.]+)$")
    re_squeezing = re.compile(
        r"\[\d+\.\d+\]\s*squeezing\s*\(l=(\d+)\):\s*S\s+size\s*=\s*(\d+)"
    )

    def flush():
        nonlocal pending_trying
        if current is not None and pending_trying is not None:
            current.steps.append((*pending_trying, -1, -1))
            pending_trying = None

    with open(path) as f:
        for raw in f:
            raw = raw.rstrip()
            m = re_thread.match(raw)
            line = m.group(1) if m else raw

            if not line:
                continue

            if "Time elapsed" in line or line.startswith("Time:"):
                continue

            if line.startswith("Doing Strategy"):
                continue

            if m := re_params.search(line):
                flush()
                if current is not None:
                    runs.append(current)
                current = PrimeRun(p=int(m.group(1)), k=int(m.group(2)))
                pending_trying = None
                continue

            if current is None:
                continue

            if m := re_step_generic.search(line):
                n = int(m.group(1))
                s_size = int(m.group(2))

                if current.step1_s_size is None and n == 1:
                    current.step1_s_size = s_size

                current.steps.append((-1, -1, n, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                pending_trying = None
                continue

            if m := re_trying_inline.search(line):
                current.steps.append(
                    (int(m.group(1)), int(m.group(2)),
                     int(m.group(3)), int(m.group(4)))
                )
                pending_trying = None
                if int(m.group(4)) == 0:
                    current.final_s_zero = True
                continue

            if m := re_forcing.search(line):
                if pending_trying is not None:
                    current.steps.append((*pending_trying, -1, -1))
                pending_trying = (int(m.group(1)), int(m.group(2)))
                continue

            if m := re_trying.search(line):
                if pending_trying is not None:
                    current.steps.append((*pending_trying, -1, -1))
                pending_trying = (int(m.group(1)), int(m.group(2)))
                continue

            if m := re_result_squeeze.search(line):
                n = int(m.group(1))
                s_size = int(m.group(2))
                current.steps.append((-1, -1, n, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                pending_trying = None
                continue

            if m := re_result_simple.search(line):
                n = int(m.group(1))
                s_size = int(m.group(2))
                current.steps.append((-1, -1, n, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                pending_trying = None
                continue

            if m := re_strategy_size.search(line):
                s_size = int(m.group(1))
                current.steps.append((-1, -1, -1, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                pending_trying = None
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

            if m := re_step_plain.match(line):
                pending_trying = None
                continue

            if m := re_squeezing.search(line):
                n = int(m.group(1))
                s_size = int(m.group(2))
                current.steps.append((-1, -1, n, s_size))
                if s_size == 0:
                    current.final_s_zero = True
                pending_trying = None
                continue

    flush()
    if current is not None:
        runs.append(current)

    return runs

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

def check_seed(seed, p, k):
    target = set(range(1, k + 1))
    for a in range(1, p):
        image = set((a * s) % p for s in seed)
        if len(image) != k:
            continue
        abs_image = set(min(x, p - x) for x in image)
        if abs_image == target:
            return True, a
    return False, None

def fractional_part(x: Fraction) -> Fraction:
    return x - math.floor(x)

def r_vector(t: Fraction, n: int, k: int) -> tuple:
    return tuple(
        math.floor(n * fractional_part(j * t))
        for j in range(1, k + 1)
    )

def all_r_vectors_rational(n: int, k: int, p: int) -> list[tuple]:
    seen = set()
    for m in range(p):
        x = Fraction(m, p)
        seen.add(r_vector(x, n, k))
    return list(seen)

def valid_resolve_modulo(p, k) -> bool:
    return set(all_r_vectors_rational(k + 1, k, k + 1)).issubset(
        set(all_r_vectors_rational(k + 1, k, p))
    )

def is_primes_enough(primes: list[int], k: int):
    uniq = sorted(set(primes))
    s = sum(math.log(p) for p in uniq)
    binom = k * (k + 1) / 2.0
    threshold = k * math.log((binom ** (k - 1)) / k)
    return s > threshold, s, threshold

# ══════════════════════════════════════════════════════════════════════════════
# OUTPUT
# ══════════════════════════════════════════════════════════════════════════════

def format_run(run: PrimeRun):
    lines = [f"  p={run.p}  k={run.k}"]
    prev_s = run.step1_s_size
    for _, _, n, s in run.steps:
        prev = f"{prev_s}" if prev_s is not None else "?"
        n_str = str(n) if n >= 0 else "?"
        lines.append(f"    Prev S={prev:>8}  =>  n={n_str:>3}  S={s:>8}")
        prev_s = s

    if prev_s == 0:
        lines.append("    => S=0  (success)")
        return True, "\n".join(lines)

    if run.counter_example:
        if not is_prime(run.k + 1):
            lines.append("    Cannot resolve remaining seed (k+1 not prime)")
            return False, "\n".join(lines)
        if not valid_resolve_modulo(run.p, run.k):
            lines.append("    Invalid resolve modulo")
            return False, "\n".join(lines)
        results = [check_seed(seed, run.p, run.k) for seed in run.seeds]
        ok = all(r[0] for r in results)
        lines.append(f"    ** REMAINING SEEDS [{'resolvable' if ok else 'UNRESOLVABLE'}] **")
        return ok, "\n".join(lines)

    return False, "\n".join(lines + ["    END OF BLOCK - UNKNOWN RESULT"])

def print_results(runs: list[PrimeRun], label: str, detail: bool = True):
    if not runs:
        print("No parameter blocks found.")
        return

    k = runs[0].k
    formatted = [format_run(r) for r in runs]

    if detail:
        print("=" * 60)
        print(f"PER-PRIME DETAIL  [{label}]")
        print("=" * 60)
        for _, txt in formatted:
            print(txt)
            print()

    valid = [r.p for r, (ok, _) in zip(runs, formatted) if ok]
    invalid = [r.p for r, (ok, _) in zip(runs, formatted) if not ok]
    ok, s, thresh = is_primes_enough(valid, k)

    print("=" * 60)
    print(f"PROOF SUMMARY  [{label}]")
    print("=" * 60)
    print(f"k = {k}")
    print(f"valid primes   = {sorted(set(valid))}")
    print(f"invalid primes = {sorted(set(invalid))}")
    print(f"log prod       = {s:.6f}")
    print(f"threshold      = {thresh:.6f}")
    print(f"PROOF: {'YES' if ok else 'NO'}")

# ══════════════════════════════════════════════════════════════════════════════
# MAIN
# ══════════════════════════════════════════════════════════════════════════════

def main():
    if len(sys.argv) != 2:
        print(f"usage: python {os.path.basename(__file__)} <logfile|folder>")
        sys.exit(1)

    input_path = sys.argv[1]
    log_files = collect_log_files(input_path)

    if os.path.isdir(input_path):
        all_runs = []
        for path in log_files:
            runs = parse_log(path)
            if runs:
                print_results(runs, os.path.basename(path))
                print()
                all_runs.extend(runs)
        if len(all_runs) > 1:
            print_results(all_runs, "ALL FILES", detail=False)
    else:
        print_results(parse_log(input_path), os.path.basename(input_path))

if __name__ == "__main__":
    main()
