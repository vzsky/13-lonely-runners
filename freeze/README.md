# Freezer: retaining intended behavior
## aka regression test i guess

- frozen file is generated so to "freeze" the new code with a previous version of new code. 
- doesn't do anything meaningful except keeping my own mind at peace (sometimes)
- nine-and-ten is a lot more useful / convincing

## Nine and ten freezing
- freshly clone and patch the original nine and ten runner code to collapse using our new canonical representation.
- run both code and expect the same number of tuples in each step.
<<<<<<< HEAD
=======
- the changes from the original code are in `.diff`. hopefully small and direct enough to verify by hand
### How to use 
- run `generate_old.sh` once 
- run `generate_new.sh` for every change to lib (`/src`)
- then run `nine_freezer.py` to check if the changes maintain existing structure.

>>>>>>> 73100c2 (add freezer)
