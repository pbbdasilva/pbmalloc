#ifndef PB_MALLOC_PBMALLOC_H
#define PB_MALLOC_PBMALLOC_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <math.h>
#include <unistd.h>

// >= multiple of 32. then, result >= 32
#define NUM_BINS 128
#define BIN_SIZE 32
#define ALIGN32(x) (((x + BIN_SIZE - 1) >> 5) << 5)
#define PAGE_SIZE 4096
#define ALIGNPAGE(x) (((x + PAGE_SIZE - 1) >> 12) << 12)

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct Chunk {
    size_t size;
    size_t page_nr;
    struct Chunk* next;
    struct Chunk* prev;
    int free;
} Chunk;

#define METADATA_SIZE sizeof(Chunk)

void* pbmalloc(size_t size);
void pbfree(void* ptr);

#endif //PB_MALLOC_PBMALLOC_H
