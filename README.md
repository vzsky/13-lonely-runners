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
- `article/` the source of the article.

# Remarks

The implementation doesn't exactly match what described in the article.
The article describes what ideal implementation is like. That work can be found in the `develop` branch.

We split the work into two parts, `find_cover` and `lift`. 
The first part finds $I(k, p, 1)$ and the second part find $I(k, p, c)$ from result of the first part. 
Files are split into multiple cpp, hopefully with clear enough names.
The main driver of the program is in `main.cpp`, it is where to modify to select $k$ and $p$ the program uses.

`lift` is mostly copied over from Trakulthongchai's previous work. 
`find_cover` has a few modification. First, as mentioned in the article, we can fix the first coordinate to 1.
We then generate all second coordinate from $[2, P/2]$, and search the remaining coordinate from $[2, P/2]$ using the same 
idea as Trakulthongchai's previous work. 

This means that `find_cover` generates $k$ times the equivalent classes. We did not do any class collapsing and pass 
everything onto the second phase: `lift`. In this phase, the remaining *seeds* after all the small lifts can be a set 
of $k$ seeds. We just write that to file. 

Python script will pick up the log file. Check the equivalent class and recognize a set as valid if it is. Even though we 
write counter example to log file.

A better version of this can be seen in branch `develop`
