# DeDupe

## Directions

- To run the code simply type `make run` in the terminal

- To test the code use `make test`

The Makefile is the central automation control. There are several targets in the Makefile including `run`, `build`, `clean`, and `test`. 

Running `make test` is probably what you want since that was the only target from the original source provided. The other targets can be used at your convenience.

Right now, when you run the code you'll see lots of warnings, this is because of the compiler flags set for compilation. Feel free eliminate warnings so the code works as expected (generally C devs treat warnings as errors with -Werror).

## What are we doing?

In case you haven't seen the Project #2 description here is the summary quote:
> In this project, you will speed up a tool that detects duplicate content inside large files. The tool divides an input file into fixed-size blocks called chunks, computes a cryptographic hash for each chunk, and determines which chunks are identical. Your goal is to implement a parallel version of the baseline program using multithreading while preserving the correctness of the output.
