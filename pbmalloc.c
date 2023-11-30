//
// Created by pedro on 28/11/23.
//

#include "pbmalloc.h"

void* heap_base = NULL;

static inline int FREE_CHUNK(Chunk* ptr) {
    return ptr->free == 1;
}

static inline int WIDE_ENOUGH(Chunk* ptr, size_t size) {
    return ptr->size >= size;
}

Chunk* find_block(Chunk** last, size_t size) {
    Chunk* ptr = heap_base;

    // search for free chunk with enough size
    while (ptr != NULL) {
        if (FREE_CHUNK(ptr) && WIDE_ENOUGH(ptr, size)) break;

        *last = ptr;
        ptr = ptr->next;
    }

    return ptr;
}

Chunk* extend_heap(Chunk* last, size_t size) {
    void* break_ptr = sbrk(0);

    // check for failed sbrk call
    if (sbrk(METADATA_SIZE + size) == (void *) -1) return NULL;

    // initialize block
    Chunk* new_block = break_ptr;
    new_block->size = size;
    new_block->next = NULL;
    if (last != NULL) {
        last->next = new_block;
        new_block->prev = last;
    }
    new_block->free = 0;

    return new_block;
}

void split_block(Chunk* chunk, size_t size) {
    // create free chunk
    Chunk* spare_chunk = (void*) chunk + (size + METADATA_SIZE);
    spare_chunk->size = chunk->size - size - METADATA_SIZE;
    spare_chunk->next = chunk->next;
    spare_chunk->prev = chunk;
    spare_chunk->free = 1;

    // update original chunk
    chunk->next = spare_chunk;
    chunk->size = size;
}

void merge_blocks(Chunk* first_chunk, Chunk* second_chunk) {
    first_chunk->size += second_chunk->size + METADATA_SIZE;
    first_chunk->next = second_chunk->next;
    if (second_chunk->next != NULL) second_chunk->next->prev = first_chunk;
}

void* pbmalloc(size_t size) {
    assert(size >= 0);
    assert(ALIGN4(size) % 4 == 0);
    assert(ALIGN4(size) >= size);

    size = ALIGN4(size);

    Chunk* curr = NULL;
    Chunk* last = NULL;

    if (heap_base != NULL) {
        last = heap_base;
        curr = find_block(&last, size);

        if (curr != NULL) {
            assert(curr->free == 1);
            // check if splittable
            if (curr->size - size >= METADATA_SIZE + 4) split_block(curr, size);
            curr->free = 0;
        } else {
            curr = extend_heap(last, size);
            if (curr == NULL) return NULL;
        }
    } else {
        curr = extend_heap(NULL, size);
        if (curr == NULL) return NULL;
        heap_base = curr;
    }

    // curr points to metadata block, we need to skip it to return actual memory
    void* data_ptr = (void*) curr + METADATA_SIZE;
    return data_ptr;
}

void pbfree(void* ptr) {
    if (ptr == NULL) return;
    if (heap_base == NULL) return;
    if (ptr < heap_base || ptr > sbrk(0)) return;

    void* metadata_ptr = ptr - METADATA_SIZE;
    Chunk * chunk_ptr = (Chunk *) metadata_ptr;

    chunk_ptr->free = 1;
    if (chunk_ptr->next != NULL && FREE_CHUNK(chunk_ptr->next)) merge_blocks(chunk_ptr, chunk_ptr->next);
    if (chunk_ptr->prev != NULL && FREE_CHUNK(chunk_ptr->prev)) merge_blocks(chunk_ptr->prev, chunk_ptr);
}
