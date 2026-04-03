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

------------------------------------------------------------
STAGE INTERFACES (TEAM CONTRACT)
------------------------------------------------------------
Stage1 -> Stage2:
- Input: (input_path, chunk_size, read_mode)
- Output: chunks[] with {chunk_id, offset, length, data_ref}

Stage2 -> Stage3:
- Input: chunks[]
- Output: hash_results[] indexed by chunk_id

Stage3 -> Final:
- Input: hash_results[]
- Output: duplicate_groups[] + output file content

Stage4 (cross-cutting):
- Synchronization primitives + error handling + tests + benchmarks
