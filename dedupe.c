// OPTION 2 PSEUDOCODE DESIGN DOCUMENT
// PROGRAM DeDupePipeline  // pseudocode only, no implementation

// DATA STRUCTURES
// - Config:
//     input_path
//     output_path
//     chunk_size
//     num_threads
//     read_mode            // "mmap" or "fread"
// - Chunk:
//     chunk_id
//     byte_offset
//     length
//     data_ref
// - HashResult:
//     chunk_id
//     hash_value
// - DuplicateGroup:
//     hash_value
//     chunk_ids[]
// - PipelineState:
//     chunks[]
//     hash_results[]       // indexed by chunk_id to preserve order
//     duplicate_groups[]
//     stage1_done
//     stage2_done
//     stage3_done
//     error_state

// MAIN
// 1. config <- ParseCLIArguments()
// 2. ValidateConfig(config)
// 3. state <- InitializePipelineState()

// 4. Start Stage 4 Coordinator
//    - create synchronization primitives
//    - start worker threads where needed
//    - monitor stage completion / error signals

// 5. Stage 1 (Member 1): File Reading & Chunking
//    - chunks <- BuildChunks(config.input_path, config.chunk_size, config.read_mode)
//    - state.chunks <- chunks
//    - signal stage1_done

// 6. Stage 2 (Member 2): Parallel Hashing
//    - wait until stage1_done
//    - hash_results <- ComputeHashesInParallel(state.chunks, config.num_threads)
//    - state.hash_results <- hash_results
//    - signal stage2_done

// 7. Stage 3 (Member 3): Duplicate Detection & Output
//    - wait until stage2_done
//    - duplicate_groups <- DetectDuplicates(state.hash_results)
//    - state.duplicate_groups <- duplicate_groups
//    - WriteDuplicateMap(config.output_path, state.duplicate_groups)
//    - signal stage3_done

// 8. Stage 4 (Member 4): Final Integration Checks
//    - wait until stage3_done
//    - verify output format and ordering
//    - teardown resources
//    - print summary and timing

// 9. Exit success / failure


// ------------------------------------------------------------
// MEMBER 1 MODULE: FILE READING & CHUNKING
// ------------------------------------------------------------
// FUNCTION BuildChunks(input_path, chunk_size, read_mode) RETURNS chunks[]
// 1. file_size <- GetFileSize(input_path)
// 2. IF read_mode == "mmap":
//       file_ref <- MapFileToMemory(input_path)
//    ELSE:
//       file_ref <- OpenFileStream(input_path)

// 3. total_chunks <- CeilDivide(file_size, chunk_size)
// 4. chunks <- AllocateChunkArray(total_chunks)

// 5. FOR chunk_id in [0 .. total_chunks-1]:
//       offset <- chunk_id * chunk_size
//       length <- Min(chunk_size, file_size - offset)   // handles uneven final chunk
//       chunks[chunk_id] <- CreateChunk(chunk_id, offset, length, file_ref)

// 6. ValidateChunksCoverFullFile(chunks, file_size)
// 7. Return chunks

// NOTES (ownership)
// - Handles empty files, tiny files, and non-even chunk division.
// - Exposes chunk array with stable chunk_id ordering.
// - No hashing logic here.


// ------------------------------------------------------------
// MEMBER 2 MODULE: PARALLEL HASHING
// ------------------------------------------------------------
// FUNCTION ComputeHashesInParallel(chunks[], num_threads) RETURNS hash_results[]
// 1. work_queue <- BuildWorkQueue(chunks)
// 2. hash_results <- AllocateHashResultArray(size = len(chunks))  // indexed by chunk_id
// 3. thread_pool <- CreateThreadPool(num_threads)

// 4. FOR each worker in thread_pool:
//       worker runs HashWorker(work_queue, hash_results)

// 5. WaitForAllWorkers(thread_pool)
// 6. Return hash_results

// FUNCTION HashWorker(work_queue, hash_results[])
// 1. LOOP:
//       chunk <- PopNextChunk(work_queue)
//       IF chunk == NONE: BREAK
//       h <- ComputeChunkHash(chunk.data_ref, chunk.length)
//       hash_results[chunk.chunk_id] <- HashResult(chunk.chunk_id, h)

// NOTES (ownership)
// - Owns load balancing strategy (static/dynamic scheduling).
// - Preserves deterministic chunk order via direct index writes.
// - Focused on throughput and thread-safe result publication.


// ------------------------------------------------------------
// MEMBER 3 MODULE: DUPLICATE DETECTION & OUTPUT
// ------------------------------------------------------------
// FUNCTION DetectDuplicates(hash_results[]) RETURNS duplicate_groups[]
// 1. method <- ChooseStrategy("hash_map" OR "sort_by_hash")
// 2. IF method == "hash_map":
//       table <- BuildHashToChunkListMap(hash_results)
//       groups <- ExtractOnlyRepeatedEntries(table)
//    ELSE:
//       sorted <- SortByHashValue(hash_results)
//       groups <- GroupAdjacentEqualHashes(sorted)

// 3. Return groups

// FUNCTION WriteDuplicateMap(output_path, duplicate_groups[])
// 1. out <- OpenOutputFile(output_path)
// 2. WriteHeaderIfRequired(out)
// 3. FOR each group in duplicate_groups:
//       WriteGroupInReferenceFormat(out, group)
// 4. Close(out)

// NOTES (ownership)
// - Ensures exact output format and ordering rules.
// - Handles collisions/verification policy if required by spec.
// - No file chunking or threading logic here.


// ------------------------------------------------------------
// MEMBER 4 MODULE: SYNCHRONIZATION, INTEGRATION & TESTING
// ------------------------------------------------------------
// FUNCTION Coordinator(state)
// 1. init barriers/condition_variables/message_queues
// 2. launch stage threads or orchestrate staged execution
// 3. propagate stop signal on any error
// 4. collect per-stage timing metrics
// 5. ensure clean shutdown and resource release

// FUNCTION RunIntegrationTests()
// 1. test empty file
// 2. test single chunk file
// 3. test non-even final chunk
// 4. test all unique chunks
// 5. test many duplicate chunks
// 6. test deterministic output order
// 7. compare produced output to reference output byte-for-byte

// FUNCTION RunBenchmarks()
// 1. vary file sizes and chunk sizes
// 2. vary thread counts
// 3. record stage timings and total speedup
// 4. report scalability bottlenecks

// NOTES (ownership)
// - Owns interfaces/contracts between stages.
// - Validates correctness and performance claims.
// - Maintains final integrated `dedupe.c` build behavior.


// ------------------------------------------------------------
// STAGE INTERFACES (TEAM CONTRACT)
// ------------------------------------------------------------
// Stage1 -> Stage2:
// - Input: (input_path, chunk_size, read_mode)
// - Output: chunks[] with {chunk_id, offset, length, data_ref}

// Stage2 -> Stage3:
// - Input: chunks[]
// - Output: hash_results[] indexed by chunk_id

// Stage3 -> Final:
// - Input: hash_results[]
// - Output: duplicate_groups[] + output file content

// Stage4 (cross-cutting):
// - Synchronization primitives + error handling + tests + benchmarks

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "hash_functions.h"

int compare_hashes(unsigned char *a, unsigned char *b, int n) {
	for(int i=0; i < n; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}

typedef struct {
	int *slots;         
    int table_size;
    int mask;           
} HashTable;

HashTable* hashtable_create(int n_hashes) {
	size_t table_size = 16;
	while (table_size < (size_t)n_hashes * 2) {
		table_size = table_size << 1;
	}

	HashTable* tablePtr = malloc(sizeof(HashTable));
	tablePtr->table_size = table_size;
	// we'll use this to compute index with AND instead of % (faster)
	// starting at 0 for bitwise ops
	tablePtr->mask = table_size - 1;

	// index for hash
	tablePtr->slots = malloc(table_size * sizeof(int));
	for (int i = 0; i < table_size; i++) {
		tablePtr->slots[i] = -1;
	}

	return tablePtr;
}

void hashtable_destroy(HashTable *table) {
	free(table->slots);
	free(table);
}

int hashtable_find_or_insert(HashTable *table, int chunk_id, unsigned char **hashes, int hash_size) {
	uint64_t key;
	memcpy(&key, hashes[chunk_id], sizeof(uint64_t));
	// this is the bitwise mentioned before, faster than %
	int slot = (int)(key & (uint64_t)table->mask);

	// probe the table in a loop
	while(1) {
		if(table->slots[slot] == -1) {
		// slot is empty, populate
		table->slots[slot] = chunk_id;
		return 0;
	}
	else {
		// if hash location has already filled, move to next slot
		if(memcmp(hashes[chunk_id], hashes[table->slots[slot]], hash_size)) {
			return 1; 
		}
		slot = (slot + 1) & table->mask;
		}
	}
}

void detect_duplicates(unsigned char **hashes, int n_hashes, int hash_size, char *output) {
	FILE *fp = fopen(output, "w");

	// zero chunks edge case
	if(n_hashes == 0) {
		fprintf(fp, "\n");
		fclose(fp);
	}

	HashTable *table = hashtable_create(n_hashes);
	int *mask = malloc(n_hashes * sizeof(int));

	for(int i = 0; i <= n_hashes; i++) {
		mask[i] = hashtable_find_or_insert(table, i, hashes, hash_size);
	}

	hashtable_destroy(table);

	for(int i = 0; i <= n_hashes; i++) {
		fputc('0' + mask[i], fp);
		fputc('\n', fp);
	}

	fclose(fp);
	free(mask);
}

// Function name: dedupe
// Description:   Computes a hash for each chunk of the input file, and the obtained hashes
//                to each other to determine the number of unique chunks in the file
void dedupe(char *filename, int chunk_size, char *output) {
	FILE *fp;
	char *buffer = (char *) malloc(chunk_size*sizeof(char));
	unsigned char **hashes = NULL;
	int hash_size = size_sha512(), n_hashes = 0;

	// load chunks of the input file and hash them
	fp = fopen(filename, "r");
	assert(fp != NULL);
	while(fread(buffer, sizeof(char), chunk_size, fp) == chunk_size) {
		hashes = (unsigned char **) realloc(hashes, (n_hashes+1)*sizeof(unsigned char *));
		hashes[n_hashes] = calculate_sha512((unsigned char *)buffer, chunk_size);
		n_hashes++;
	}
	fclose(fp);

	int mask[n_hashes];
	for(int i=0; i < n_hashes; i++)
		mask[i] = 0;
	for(int i=0; i < n_hashes; i++)
		for(int j=i+1; j < n_hashes; j++)
			if(compare_hashes(hashes[i], hashes[j], hash_size)) {	
				mask[j] = 1;
				break;
			}

	// print results
	fp = fopen(output, "w");
	assert(fp != NULL);
	for(int i=0; i < n_hashes; i++)
		fprintf(fp, "%d", mask[i]);
	fprintf(fp, "\n");
	fclose(fp);

	// release stuff
	free(buffer);
	for(int i=0; i < n_hashes; i++)
		free(hashes[i]);
	free(hashes);
}
