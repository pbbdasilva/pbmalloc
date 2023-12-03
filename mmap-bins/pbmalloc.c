//
// Created by pedro on 02/12/23.
//
//TO-DO: solve >= 320 bytes allocation to behave as old allocation (split_block, etc)

#include <sys/mman.h>
#include "pbmalloc-binned.h"

Chunk* bins_ptr[NUM_BINS] = {0};

static inline int FREE_CHUNK(Chunk* ptr) {
    return ptr->free == 1;
}

static inline int WIDE_ENOUGH(Chunk* ptr, size_t size) {
    return ptr->size >= size;
}

static inline size_t PAGE_SIZE(size_t size) {
    return ALIGN32(size + METADATA_SIZE);
}

Chunk* find_block(Chunk** last, size_t size) {
    size_t page_index = (PAGE_SIZE(size)/BIN_SIZE) - 1;
    Chunk* ptr = bins_ptr[page_index];

    // search for free chunk with enough size
    while (ptr != NULL) {
        if (FREE_CHUNK(ptr) && WIDE_ENOUGH(ptr, size)) break;

        *last = ptr;
        ptr = ptr->next;
    }

    return ptr;
}

Chunk* alloc_memory(Chunk* last, size_t size) {
    void* block = mmap(NULL, PAGE_SIZE(size), PROT_WRITE | PROT_READ,
                       MAP_PRIVATE | MAP_ANONYMOUS,0, 0);
    assert(block != MAP_FAILED);
    Chunk* new_block = block;
    new_block->size = PAGE_SIZE(size) - METADATA_SIZE;
    new_block->next = NULL;
    if (last != NULL) {
        last->next = new_block;
        new_block->prev = last;
    }
    new_block->free = 0;

    return new_block;
}

void* pbmalloc(size_t size) {
    size_t page_index = (PAGE_SIZE(size) / BIN_SIZE) - 1;
    Chunk* curr = NULL;
    Chunk* last = NULL;

    if (bins_ptr[page_index] != NULL) {
        last = bins_ptr[page_index];
        curr = find_block(&last, size);

        if (curr != NULL) {
            curr->free = 0;
        } else {
            curr = alloc_memory(last, size);
            if (curr == NULL) return NULL;
        }
    } else {
        curr = alloc_memory(NULL, size);
        if (curr == NULL) return NULL;
        bins_ptr[page_index] = curr;
    }

    return (void*) curr + METADATA_SIZE;
}

void pbfree(void* ptr) {
    if (ptr == NULL) return;
    void* metadata_ptr = ptr - METADATA_SIZE;
    Chunk* chunk_ptr = (Chunk*) metadata_ptr;
    if (chunk_ptr->size == 0) return;
    chunk_ptr->free = 1;
}
