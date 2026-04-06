#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#include "hash_functions.h"

typedef struct ThreadWorkArgs {
  FILE *fp;
  int offset;
  int chunk_size;
  unsigned char **hashes;
} ThreadWorkArgs;

typedef struct ThreadPool {
  int head;
  int tail;
  ThreadWorkArgs **work_args;
  pthread_t **threads;
  int num_threads;
  int work_args_size;
  pthread_mutex_t head_lock;
  sem_t work_sem;
  atomic_int jobs_done;
  pthread_mutex_t jobs_done_lock;
} ThreadPool;

ThreadPool *ThreadPool_Create(int size, int num_threads, void* (*threadLoop)(void*)) {
  ThreadPool *tp = malloc(sizeof(ThreadPool));
  tp->head = 0;
  tp->tail = 0;
  tp->work_args_size = size;
  tp->work_args = malloc(tp->work_args_size * sizeof(ThreadWorkArgs*));
  pthread_mutex_init(&tp->head_lock, NULL);
  sem_init(&tp->work_sem, 0, 0);

  atomic_store(&tp->jobs_done, 0);
  pthread_mutex_init(&tp->jobs_done_lock, NULL);

  tp->num_threads = num_threads;
  tp->threads = malloc(num_threads * sizeof(pthread_t*));
  for (int i = 0; i < tp->num_threads; i++) {
    pthread_t *thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, threadLoop, tp);
    tp->threads[i] = thread;
  }

  return tp;
}

void ThreadPool_Destroy(ThreadPool *tp) {
  for (int i = tp->head; i < tp->tail; i++) {
    free(tp->work_args[i % tp->work_args_size]);
  }

  free(tp->work_args);

  for (int i = 0; i < tp->num_threads; i++) {
    pthread_cancel(*tp->threads[i]);
    free(tp->threads[i]);
  }

  free(tp->threads);

  pthread_mutex_destroy(&tp->head_lock);
  sem_destroy(&tp->work_sem);

  free(tp);
}

void waitForWork(ThreadPool *tp) {
  sem_wait(&tp->work_sem);
}

ThreadWorkArgs* getWorkAtHead(ThreadPool *tp) {
  pthread_mutex_lock(&tp->head_lock);
  ThreadWorkArgs *args = tp->work_args[tp->head % tp->work_args_size];
  tp->head++;
  pthread_mutex_unlock(&tp->head_lock);
  return args;
}

bool tryPutWork(ThreadPool *tp, ThreadWorkArgs *args) {
  pthread_mutex_lock(&tp->head_lock);
  if (tp->tail - tp->head < tp->work_args_size) {
    tp->work_args[tp->tail % tp->work_args_size] = args;
    tp->tail++;
    pthread_mutex_unlock(&tp->head_lock);
    sem_post(&tp->work_sem);
    return true;
  } else {
    pthread_mutex_unlock(&tp->head_lock);
    return false;
  }
}

void incrementJobs(ThreadPool *tp) {
  atomic_fetch_add(&tp->jobs_done, 1);
}

ThreadWorkArgs *ThreadWorkArgs_Create(FILE *file, int offset, int chunk_size, unsigned char **hashes) {
  ThreadWorkArgs *thread_args = malloc(sizeof(ThreadWorkArgs));

  thread_args->fp = file;
  thread_args->offset = offset;
  thread_args->chunk_size = chunk_size;
  thread_args->hashes = hashes;

  return thread_args;
}

void ThreadWorkArgs_Destroy(ThreadWorkArgs *thread_args) {
  free(thread_args);
}

int compare_hashes(unsigned char *a, unsigned char *b, int n) {
	for(int i=0; i < n; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}

uint64_t fnv1a_hash(unsigned char *data, size_t len) {
  uint64_t hash = 1469598103934665607ULL;

  for (int i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 1099511628211ULL;
  }

  return hash;
}

size_t getFileSize(FILE* fp) {

  if (fp == NULL) return -1;

  size_t pos = ftell(fp);

  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);

  fseek(fp, pos, SEEK_SET);

  return file_size;
}

void threadWork(ThreadWorkArgs *args) {
  char *buffer = malloc(args->chunk_size);

  pread(fileno(args->fp), buffer, args->chunk_size, args->offset * args->chunk_size);

  args->hashes[args->offset] = calculate_sha512(buffer, args->chunk_size);

  free(buffer);
}

void* threadLoop(void *arg) {
  ThreadPool *tp = arg;
  while (true) {
    waitForWork(tp);
    ThreadWorkArgs *work_args = getWorkAtHead(tp);
    threadWork(work_args);
    free(work_args);
    incrementJobs(tp);
  }
}

void makeDuplicatesMask(int num_elements, unsigned char **hashes_array, int hash_size, char *mask) {
  int seen[num_elements];
  memset(seen, 0, num_elements * sizeof(int));

  for (int i = 0; i < num_elements; i++) {
    int mask_index = fnv1a_hash(hashes_array[i], hash_size) % num_elements;
    if (seen[mask_index] == 1) {
      bool matched = false;
      for (int j = 0; j < i; j++) {
        if (compare_hashes(hashes_array[i], hashes_array[j], hash_size)) {
          matched = true;
          mask[i] = '1';
          break;
        }
      }

      if (matched == false) {
        seen[mask_index] = 1;
        mask[i] = '0';
      }

    } else {
      seen[mask_index] = 1;
      mask[i] = '0';
    }
  }
}

ThreadPool *thread_pool = NULL;

void dedupe(char *filename, int chunk_size, char *output) {
  FILE *fp = fopen(filename, "r");
  assert(fp != NULL);

  int hash_size = size_sha512();
  size_t file_size = getFileSize(fp);
  int num_chunks = file_size / chunk_size;
  unsigned char **hashes = malloc(num_chunks * sizeof(unsigned char*));

  thread_pool = ThreadPool_Create(20, 11, threadLoop);

  for (int i = 0; i < num_chunks; i++) {
    ThreadWorkArgs *args = ThreadWorkArgs_Create(fp, i, chunk_size, hashes);
    while(!tryPutWork(thread_pool, args));
  }

  while (true) {
    if (atomic_load(&thread_pool->jobs_done) == num_chunks) break;
  }

  ThreadPool_Destroy(thread_pool);

  fclose(fp);

  char mask[num_chunks + 1];
  mask[num_chunks] = 0;

  makeDuplicatesMask(num_chunks, hashes, hash_size, mask);

  fp = fopen(output, "w");
  assert(fp != NULL);
  fprintf(fp, "%s\n", mask);
  fclose(fp);

  for (int i = 0; i < num_chunks; i++)
    free(hashes[i]);
  free(hashes);
}