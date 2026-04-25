# LRC holds for 11, 12, 13 runners

> each in a group of up to 13 runners will eventually be lonely

## branches
- `main` contains the polished code, we checked that this proof k <= 10
- `for-k-12` is the earlier version of code. we ran this for k=11 and k=12

## Accompanying files: 
- `main.cpp` the source code 
- `run.sh` just a compilation guide
- `results/*` result of running the source
- `log_summary.py` read `result_k` and give summary the proof.
    - this check for everything we know provided that the log files are correctly generated.
    - this is entirely AI generated... but it is fine as 
    - this is for sanity check only
    - the problem this script solves is trivial enough
- `article/` the source of the article.
- `freezer/` some regression tests.

## Implementation Details
We split this into small parts with three main components: 
- `find_cover.h` handles finding $I(k, p, 1)$
- `lift.h` handles the lifting
- `main.cpp` is the driver and handle resolving $(1..k)$
