#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_functions.h"

int compare_hashes(unsigned char *a, unsigned char *b, int n) {
  for (int i = 0; i < n; i++)
    if (a[i] != b[i])
      return 0;
  return 1;
}

typedef struct {
  int *slots;
  int table_size;
  int mask;
} HashTable;

HashTable *hashtable_create(int n_hashes) {
  size_t table_size = 16;
  while (table_size < (size_t)n_hashes * 2) {
    table_size = table_size << 1;
  }

  HashTable *tablePtr = malloc(sizeof(HashTable));
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

int hashtable_find_or_insert(HashTable *table, int chunk_id,
                             unsigned char **hashes, int hash_size) {
  uint64_t key;
  memcpy(&key, hashes[chunk_id], sizeof(uint64_t));

  // this is the bitwise mentioned before, faster than %
  int slot = (int)(key & (uint64_t)table->mask);
	
  if (table->slots[slot] == -1) {
    // slot is empty, populate
    table->slots[slot] = chunk_id;

    return 0;
  }

	return 1;
}

int *detect_duplicates(unsigned char **hashes, int n_hashes, int hash_size) {

  HashTable *table = hashtable_create(n_hashes);
  int *mask = malloc(n_hashes * sizeof(int));

  for (int i = 0; i < n_hashes; i++) {
    mask[i] = hashtable_find_or_insert(table, i, hashes, hash_size);
  }
  hashtable_destroy(table);

  return mask;
}

// Function name: dedupe
// Description:   Computes a hash for each chunk of the input file, and the
// obtained hashes
//                to each other to determine the number of unique chunks in the
//                file
void dedupe(char *filename, int chunk_size, char *output) {
  FILE *fp;
  char *buffer = (char *)malloc(chunk_size * sizeof(char));
  unsigned char **hashes = NULL;
  int hash_size = size_sha512(), n_hashes = 0;

  // load chunks of the input file and hash them
  fp = fopen(filename, "r");
  assert(fp != NULL);
  while (fread(buffer, sizeof(char), chunk_size, fp) == chunk_size) {
    hashes = (unsigned char **)realloc(hashes, (n_hashes + 1) *
                                                   sizeof(unsigned char *));
    hashes[n_hashes] = calculate_sha512((unsigned char *)buffer, chunk_size);
    n_hashes++;
  }
  fclose(fp);

  int *output_mask = detect_duplicates(hashes, n_hashes, hash_size);

  fp = fopen(output, "w");
  assert(fp != NULL);
  for (int i = 0; i < n_hashes; i++)
    fprintf(fp, "%d", output_mask[i]);

  fprintf(fp, "\n");
  fclose(fp);

  // release stuff
  free(buffer);
  for (int i = 0; i < n_hashes; i++)
    free(hashes[i]);
  free(hashes);
  free(output_mask);
}
