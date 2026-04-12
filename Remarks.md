# Implementation

The implementation doesn't exactly match what described in the article. 

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
