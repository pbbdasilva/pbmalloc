//
// Created by pedro on 02/12/23.
//

#ifndef PB_MALLOC_PBMALLOC_BINNED_H
#define PB_MALLOC_PBMALLOC_BINNED_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

// >= multiple of 32. then, result >= 32
#define BIN_SIZE 32
#define NUM_BINS 10
#define ALIGN32(x) (((x+31) >> 5) << 5)

typedef struct Chunk {
    size_t size;
    struct Chunk* next;
    struct Chunk* prev;
    int free;
} Chunk;

#define METADATA_SIZE sizeof(Chunk)

void* pbmalloc(size_t size);
void pbfree(void* ptr);

#endif //PB_MALLOC_PBMALLOC_BINNED_H
