#!/usr/bin/env python3

import re
import os
import sys
import math
from collections import defaultdict

def parse_seeds_from_line(line):
    seeds = re.findall(r'\(\s*([\d\s]+?)\s*\)', line)
    return [list(map(int, s.split())) for s in seeds] if seeds else None

def check_seed(seed, p, k):
    target = set(range(1, k + 1))
    for a in range(1, p):
        image = set((a * s) % p for s in seed)
        if len(image) != k:
            continue
        abs_image = set(min(x, p - x) for x in image)
        if abs_image == target:
            signs = {min(x, p-x): ('+' if x <= p//2 else '-') for x in image}
            return True, a, signs
    return False, None, {}

def parse_log(text):
    blocks = []
    current_p = None
    current_k = None
    pending_seeds = []

    def commit():
        nonlocal pending_seeds
        if current_p is not None and current_k is not None and pending_seeds:
            blocks.append((current_p, current_k, pending_seeds))
        pending_seeds = []

    for line in text.splitlines():
        m = re.search(r'Parameters:\s*p\s*=\s*(\d+),\s*k\s*=\s*(\d+)', line)
        if not m:
            m = re.search(r'\bp\s*=\s*(\d+)\s*,\s*k\s*=\s*(\d+)', line)

        if m:
            commit()
            current_p = int(m.group(1))
            current_k = int(m.group(2))
            continue

        seeds = parse_seeds_from_line(line)
        if seeds and current_p is not None and current_k is not None:
            pending_seeds.extend(seeds)
            continue

        if 'trying' in line or 'Counter Example' in line or 'S size = 0' in line:
            commit()

    commit()
    return blocks

def print_prime_summary(blocks):
    primes_by_k = defaultdict(set)
    for (p, k, _) in blocks:
        primes_by_k[k].add(p)

    print(f"\n{'='*60}")
    print("Prime summary by K:")
    for k in sorted(primes_by_k):
        unique_primes = sorted(primes_by_k[k])
        log_sum = sum(math.log(p) for p in unique_primes)
        print(f"\n  K={k}: {len(unique_primes)} unique primes, sum(log(p)) = {log_sum:.4f}")
        print(f"  primes = {unique_primes}")

def main():
    if len(sys.argv) != 2:
        print("Usage: python check_seeds.py <logfile | directory>")
        sys.exit(1)

    path = sys.argv[1]
    texts = []

    if os.path.isdir(path):
        for root, _, files in os.walk(path):
            for name in files:
                try:
                    with open(os.path.join(root, name), 'r') as f:
                        texts.append(f.read())
                except Exception:
                    pass
    else:
        with open(path, 'r') as f:
            texts.append(f.read())

    if not texts:
        print("No readable log files found.")
        return

    blocks = []
    for text in texts:
        blocks.extend(parse_log(text))

    if not blocks:
        print("No seed blocks found.")
        return

    total_seeds = 0
    all_pass = True

    for (p, k, seeds) in blocks:
        print(f"\n{'='*60}")
        print(f"p={p}, k={k}: found {len(seeds)} seeds")

        if len(seeds) != k:
            print(f"  [FAIL] Expected {k} seeds, got {len(seeds)}")
            all_pass = False
        else:
            print(f"  [PASS] Seed count = {k}")

        for i, seed in enumerate(seeds):
            total_seeds += 1
            if len(seed) != k:
                print(f"  [FAIL] Seed {i}: expected length {k}, got {len(seed)}: {seed}")
                all_pass = False
                continue

            ok, a, signs = check_seed(seed, p, k)
            if ok:
                signed = ' '.join(f"{signs[j]}{j}" for j in range(1, k+1))
                print(f"  [PASS] Seed {i}: {seed}  =>  a={a}, image={{{signed}}}")
            else:
                print(f"  [FAIL] Seed {i}: {seed}  =>  no valid a found")
                all_pass = False

    print(f"\n{'='*60}")
    print(f"Summary: {len(blocks)} blocks, {total_seeds} seeds checked")
    print(f"Overall: {'ALL PASS' if all_pass else 'SOME FAILURES'}")

    print_prime_summary(blocks)

if __name__ == '__main__':
    main()
