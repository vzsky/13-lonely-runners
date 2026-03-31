# LRC holds for 11, 12, 13 runners

> each in a group of up to 13 runners will eventually be lonely

## Accompanying files: 
- `main.cpp` the source code 
- `run.sh` just a compilation guide
- `results/result_{k+1}_{description}` result of running the source
    - for $k=11$ it's a combination of many runs
    - for $k=11$ and $k=13$ see the `raw` directory
- `log_verifier.py` read `result_k` and verify the proof (size of primes).
- `log_summary.py` read `result_k` and give summary the proof (size of primes).
- `resolver.py` read `result_k` and 'resolve' the remaining seeds (for $k+1$ prime)
- `article/` the source of the article.
