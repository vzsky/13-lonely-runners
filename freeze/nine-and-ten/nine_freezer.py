#!/usr/bin/env python3
import re
import sys

def read_new_log(text):
    param_re = re.compile(r"Parameters:\s*p\s*=\s*(\d+)\s*,\s*k\s*=\s*(\d+)")
    step1_re = re.compile(r"Step 1.*S size\s*=\s*(\d+)")
    step20_re = re.compile(r"\[2\.0\].*S(?:\.size\(\)| size)\s*=\s*(\d+)")
    step21_re = re.compile(r"\[2\.1\].*S(?:\.size\(\)| size)\s*=\s*(\d+)")

    rows = []
    cur = None

    for line in text.splitlines():
        m = param_re.search(line)
        if m:
            if cur:
                rows.append(cur)
            cur = {"p": int(m.group(1)), "k": int(m.group(2)), "step1": None, "step2_0": None, "step2_1": None}
            continue

        if not cur:
            continue

        m = step1_re.search(line)
        if m:
            cur["step1"] = int(m.group(1))
            continue

        m = step20_re.search(line)
        if m:
            cur["step2_0"] = int(m.group(1))
            continue

        m = step21_re.search(line)
        if m:
            cur["step2_1"] = int(m.group(1))
            continue

    if cur:
        rows.append(cur)

    return rows


def read_old_log(text):
    param_re = re.compile(r"Parameters:\s*p\s*=\s*(\d+)\s*,\s*k\s*=\s*(\d+)")
    s15_re = re.compile(r"Step1\.5.*S size\s*=\s*(\d+)")
    s1_re = re.compile(r"^Step1(?!\.5)\b.*S size\s*=\s*(\d+)", re.M)
    s2_re = re.compile(r"Step2.*T size\s*=\s*(\d+)")
    s3_re = re.compile(r"Step3.*U size\s*=\s*(\d+)")

    rows = []
    cur = None

    for line in text.splitlines():
        m = param_re.search(line)
        if m:
            if cur:
                rows.append(cur)
            cur = {"p": int(m.group(1)), "k": int(m.group(2)), "step1": None, "step1_5": None, "step2": None, "step3": None}
            continue

        if not cur:
            continue

        m = s1_re.search(line)
        if m:
            cur["step1"] = int(m.group(1))
            continue

        m = s15_re.search(line)
        if m:
            cur["step1_5"] = int(m.group(1))
            continue

        m = s2_re.search(line)
        if m:
            cur["step2"] = int(m.group(1))
            continue

        m = s3_re.search(line)
        if m:
            cur["step3"] = int(m.group(1))
            continue

    if cur:
        rows.append(cur)

    return rows

def to_csv(rows, cols):
    data=",".join(cols)
    for r in rows:
        data += (",".join(str(r.get(c)) for c in cols)) + "\n" 
    return data

def translater (rows, cols):
    data = []
    for r in rows:
        data.append([str(r.get(c)) for c in cols])
    return data

def find_mismatch(a, b):
    n = min(len(a), len(b))

    if len(a) != len(b):
        print("Length mismatch:", len(a), len(b))
        return

    for i in range(n):
        if a[i] != b[i]:
            for j, (x, y) in enumerate(zip(a[i], b[i])):
                if x != y:
                    print(f"Mismatch at row {i}, col {j}: new={x}, old={y}")
                    return
            print(f"Row mismatch at {i}:")
            print("new:", a[i])
            print("old:", b[i])
            return

    print("No mismatch found")

def main():
    with open("new_nine", "r") as f:
        new_text = f.read()
    with open("orig_nine", "r") as f:
        orig_text = f.read()

    new_data = read_new_log(new_text)
    old_data = read_old_log(orig_text)

    print("=== NEW LOG ===")
    print(to_csv(new_data, ["k", "p", "step1", "step2_0", "step2_1"]))

    print("\n=== OLD LOG ===")
    print(to_csv(old_data, ["k", "p", "step1", "step1_5", "step2", "step3"]))

    csv_new = translater(new_data, ["k", "p", "step1", "step2_0", "step2_1"])
    csv_old = translater(old_data, ["k", "p", "step1_5", "step2", "step3"])
    
    if csv_new == csv_old: 
        print("PASS")
    else :
        print("FAILED - output mismatch")
        find_mismatch(csv_new, csv_old)
        exit(1)

if __name__ == "__main__":
    main()
