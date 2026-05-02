# SAT-based find cover

I thought SAT should make things faster although some prior investigations due to Rosenfeld and Silver showed it's not,
for Rosenfeld's implementation. 
Our find cover is still adhoc and prune conditions are arbitrary, so expected improvement. 

Our approach here is using SAT solver to find *all* assignments of boolean,
which maps to all speed sets that will pass the first step, the one in `find_cover.h`. 

Turned out it is a lot slower. 
One explanation I have is that it learns too much. As its goal is to quickly satisfy or reject, it becomes slower to find all assignments.
But also could us replicate certain conflict driven approach to speed this up? 
Would it be significant enough to reduce a 1.5 *machine-years* computation needed for k=14 down? 
