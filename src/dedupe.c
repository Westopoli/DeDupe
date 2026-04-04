#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "hash_functions.h"

#define MAX_THREADS 11
#define QUEUE_SIZE 100

typedef struct {
    char *data;
    int size;
    int pos;
} Chunk;

typedef struct {
    Chunk *items[QUEUE_SIZE];
    int head, tail, count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Queue;

Queue queue;

void queue_init(Queue *q) {
    q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void enqueue(Queue *q, Chunk *item) {
    pthread_mutex_lock(&q->lock);

    while (q->count == QUEUE_SIZE)
        pthread_cond_wait(&q->not_full, &q->lock);

    q->items[q->tail] = item;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

Chunk *dequeue(Queue *q) {
    pthread_mutex_lock(&q->lock);

    while (q->count == 0)
        pthread_cond_wait(&q->not_empty, &q->lock);

    Chunk *item = q->items[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    return item;
}

unsigned char **hashes = NULL;
int n_hashes = 0;

pthread_mutex_t chunklock;
pthread_mutex_t hashlock;


void *hasher(void *arg) {
    while (1) {
        Chunk *chunk = dequeue(&queue);
        
        if (chunk == NULL)
            break;
        
        unsigned char *newhash = calculate_sha512((unsigned char *)chunk->data, chunk->size);
        
        pthread_mutex_lock(&hashlock);
		hashes[chunk->pos] = newhash;
        pthread_mutex_unlock(&hashlock);
        
        free(chunk->data);
        free(chunk);
        
    }

    return NULL;
}

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
  int slot = (int)(key & (uint64_t)table->mask);
    
  // Linear Probing: loop as long as the slot is occupied
  while (table->slots[slot] != -1) {
    
    if (memcmp(hashes[chunk_id], hashes[table->slots[slot]], hash_size) == 0) {
      return 1; // True duplicate found
    }
    
    slot = (slot + 1) & table->mask;
  }

  table->slots[slot] = chunk_id;

  return 0; // Unique chunk, successfully inserted
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
  
    FILE *fp = fopen(filename, "r");
    
    assert(fp != NULL);

    queue_init(&queue);

    pthread_t threads[MAX_THREADS];


    int hash_size = size_sha512();
    int i;
    
    pthread_mutex_init(&chunklock, NULL);
    pthread_mutex_init(&hashlock, NULL);

    for (i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, hasher, NULL);
    }

    int pos = 0;

    while (1) {
        char *buffer = malloc(chunk_size);
        int read = fread(buffer, sizeof(char), chunk_size, fp);

        if (read <= 0) {
            
            free(buffer);
            break;
        }

        hashes = (unsigned char **) realloc(hashes, (n_hashes+1)*sizeof(unsigned char *));
        n_hashes++;

        Chunk *chunk = malloc(sizeof(Chunk));
        chunk->data = buffer;
        chunk->size = read;
        chunk->pos = pos++;

        enqueue(&queue, chunk);
    }

    fclose(fp);

    for (i = 0; i < MAX_THREADS; i++) {
        enqueue(&queue, NULL);
    }

    for (i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }


  int *output_mask = detect_duplicates(hashes, n_hashes, hash_size);

  fp = fopen(output, "w");
  assert(fp != NULL);
  for (int i = 0; i < n_hashes; i++)
    fprintf(fp, "%d", output_mask[i]);

  fprintf(fp, "\n");
  fclose(fp);

  for (int i = 0; i < n_hashes; i++)
    free(hashes[i]);
  free(hashes);
  free(output_mask);
}
