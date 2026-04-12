# LRC holds for 11, 12, 13 runners

> each in a group of up to 13 runners will eventually be lonely

## branches
- `main` will be frozen once the article is out.
- `develop` will be for whatever comes after that.

## Accompanying files: 
- `main.cpp` the source code 
- `run.sh` just a compilation guide
- `results/*` result of running the source
    - can be a combination of many runs
    - for $k=11$ and $k=13$ see the `raw` directory
- `log_summary.py` read `result_k` and give summary the proof.
    - this check for everything we know provided that the log files are correctly generated.
    - this is entirely AI generated... but it is fine as 
    - this is for sanity check only
    - the problem this script solves is trivial enough
- `Remarks.md` explanation of this work in more detail
- `article/` the source of the article.
