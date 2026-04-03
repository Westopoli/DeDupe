## DeDupe

The goal of this project is to optimize the performance using a paralleled approach facilitated by pthreads and semaphores.

State Trace:
- Break file into fixed sized chunks
- Compute crypto hash for each chunk
- Determine which chunks are identical

### Rules

- only change code in the `dedupe.c` file
- only `pthread` and `semaphore` libraries are allowed

## Flow

<img width="200" height="460" alt="dedupe_function_sequential_state_flow" src="https://github.com/user-attachments/assets/b708a7b9-c47e-40a3-8961-6367fa42f6b5" />
