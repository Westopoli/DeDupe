## DeDupe

The goal of this project is to optimize the performance using a paralleled approach facilitated by pthreads and semaphores.

State Trace:
- Break file into fixed sized chunks
- Compute crypto hash for each chunk
- Determine which chunks are identical

### Rules

- only change code in the `dedupe.c` file
- only `pthread` and `semaphore` libraries are allowed
