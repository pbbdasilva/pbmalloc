#include "pbmalloc.h"

Chunk* bins_ptr[NUM_BINS] = {0};

static inline int WIDE_ENOUGH(Chunk* ptr, size_t size) {
    return ptr->size >= size;
}

static inline size_t BIN_ALIGN(size_t size) {
    return ALIGN32(size + METADATA_SIZE);
}

static inline size_t PAGE_ALIGN(size_t size) {
    return ALIGNPAGE(size + METADATA_SIZE);
}

Chunk* find_block(Chunk** last, size_t size) {
    size_t page_index = MIN((BIN_ALIGN(size) / BIN_SIZE) - 1, NUM_BINS - 1);
    Chunk* ptr = bins_ptr[page_index];

    // search for free chunk with enough size
    while (ptr != NULL) {
        if (ptr->free == 1 && WIDE_ENOUGH(ptr, size)) break;

        *last = ptr;
        ptr = ptr->next;
    }

    return ptr;
}

void split_block(Chunk* chunk, size_t size) {
    Chunk* spare_chunk = (void*) chunk + BIN_ALIGN(size);
    spare_chunk->size = chunk->size - BIN_ALIGN(size);
    spare_chunk->next = chunk->next;
    spare_chunk->prev = chunk;
    spare_chunk->free = 1;

    chunk->next = spare_chunk;
    chunk->size = size;
}

Chunk* alloc_memory(Chunk* last, size_t size) {
    size_t allocated_size = PAGE_ALIGN(size);
    void* block = mmap(NULL, allocated_size, PROT_WRITE | PROT_READ,
                       MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (block == MAP_FAILED) return NULL;
    
    Chunk* new_block = block;
    new_block->size = allocated_size - METADATA_SIZE;
    new_block->next = NULL;
    
    if (last != NULL) {
        last->next = new_block;
        new_block->prev = last;
    }
    new_block->free = 0;

    if (BIN_ALIGN(size) <= (PAGE_SIZE >> 1)) {
        split_block(new_block, ALIGN32(size));
    }
    
    return new_block;
}

void* pbmalloc(size_t size) {
    size_t page_index = MIN((BIN_ALIGN(size) / BIN_SIZE) - 1, NUM_BINS - 1);
    Chunk* curr = NULL;
    Chunk* last = NULL;

    if (bins_ptr[page_index] != NULL) {
        last = bins_ptr[page_index];
        curr = find_block(&last, size);

        if (curr != NULL) {
            if (curr->size >= 2*BIN_ALIGN(size)) split_block(curr, ALIGN32(size));
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
