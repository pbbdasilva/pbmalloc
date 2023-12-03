//
// Created by pedro on 28/11/23.
//

#ifndef PB_MALLOC_PBMALLOC_H
#define PB_MALLOC_PBMALLOC_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

// gets closest bigger or equal multiple of 4
#define ALIGN4(x) (((((x)-1) >> 2) << 2) + 4)

typedef struct Chunk {
    size_t size;
    struct Chunk* next;
    struct Chunk* prev;
    int free;
} Chunk;

#define METADATA_SIZE sizeof(Chunk)

void* pbmalloc(size_t size);
void pbfree(void* ptr);

#endif //PB_MALLOC_PBMALLOC_H
