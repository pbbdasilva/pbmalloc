#include "pbmalloc.h"

Chunk* free_list[NUM_BINS] = {0};
size_t PAGE_COUNTER = 0;

static inline size_t BIN_ALIGN(size_t size) {
    return ALIGN32(size + METADATA_SIZE);
}

static inline size_t PAGE_INDEX(size_t size) {
    return MIN((BIN_ALIGN(size) / BIN_SIZE) - 1, NUM_BINS - 1);
}

static inline int WIDE_ENOUGH(Chunk* ptr, size_t size) {
    return ptr->size >= size;
}

static inline size_t PAGE_ALIGN(size_t size) {
    return ALIGNPAGE(size + METADATA_SIZE);
}

void split_block(Chunk* chunk, size_t size) {
    Chunk* spare_chunk = (void*) chunk + BIN_ALIGN(size);
    spare_chunk->size = chunk->size - BIN_ALIGN(size);
    spare_chunk->next = chunk->next;
    spare_chunk->prev = chunk;
    spare_chunk->free = 1;
    spare_chunk->page_nr = chunk->page_nr;

    chunk->next = spare_chunk;
    chunk->size = ALIGN32(size);
}

void append_free_list(Chunk* chunk, size_t page_index) {
    if (free_list[page_index]) {
        free_list[page_index]->next = chunk;
        chunk->next = NULL;
        chunk->prev = free_list[page_index];
        free_list[page_index] = free_list[page_index]->next;
    } else {
        free_list[page_index] = chunk;
        chunk->next = NULL;
        chunk->prev = NULL;
    }
}

Chunk* alloc_memory(size_t size) {
    size_t allocated_size = PAGE_ALIGN(size);
    void* block = mmap(NULL, allocated_size, PROT_WRITE | PROT_READ,
                       MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (block == MAP_FAILED)
        return NULL;
    
    Chunk* new_block = block;
    new_block->size = allocated_size - METADATA_SIZE;
    new_block->next = NULL;
    new_block->free = 0;
    new_block->page_nr = PAGE_COUNTER++;

    if (new_block->size >= (BIN_ALIGN(size) << 1)) {
        split_block(new_block, size);
        append_free_list(new_block->next, PAGE_INDEX(size));
    }
    
    return new_block;
}

Chunk* find_block(size_t size) {
    Chunk* ptr = free_list[PAGE_INDEX(size)];

    while (ptr != NULL) {
        if (WIDE_ENOUGH(ptr, size))
            break;
        ptr = ptr->next;
    }

    return ptr;
}

void* pbmalloc(size_t size) {
    size_t page_index = PAGE_INDEX(size);
    Chunk* curr = NULL;

    if (free_list[page_index]) {
        if (page_index != NUM_BINS - 1) {
            curr = free_list[page_index];
            free_list[page_index] = free_list[page_index]->prev;
            curr->prev = NULL;
            curr->next = NULL;

            if (curr->size >= (BIN_ALIGN(size) << 1)) {
                split_block(curr, size);
                append_free_list(curr->next, page_index);
            }
        } else {
            curr = find_block(size);
            if (curr == NULL) {
                curr = alloc_memory(size);
                if (curr == NULL)
                    return NULL;
            }
        }

    } else {
        curr = alloc_memory(size);
        if (curr == NULL)
            return NULL;
    }

    return (void*) curr + METADATA_SIZE;
}

void pbfree(void* ptr) {
    if (ptr == NULL)
        return;
    void* metadata_ptr = ptr - METADATA_SIZE;
    Chunk* chunk_ptr = (Chunk*) metadata_ptr;
    if (chunk_ptr->size == 0)
        return;

    chunk_ptr->free = 1;
    append_free_list(chunk_ptr, PAGE_INDEX(chunk_ptr->size));
}
