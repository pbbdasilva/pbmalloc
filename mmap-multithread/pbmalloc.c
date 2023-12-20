#include <stdio.h>
#include "pbmalloc.h"

static Arena* arena_list;
static pthread_mutex_t arena_list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t data_segment_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mmap_lock = PTHREAD_MUTEX_INITIALIZER;

void append_arena_list(Arena* arena) {
    pthread_mutex_lock(&arena_list_lock);
    arena->nxt_arena = arena_list;
    arena_list = arena;
    pthread_mutex_unlock(&arena_list_lock);
}

Arena* create_arena() {
    pthread_mutex_lock(&data_segment_lock);
    void* break_ptr = sbrk(0);
    if (sbrk(ARENA_SIZE) == (void *) -1) {
        printf("WTF SBRK not working\n");
        return NULL;
    }
    pthread_mutex_unlock(&data_segment_lock);

    Arena* arena_ptr = (Arena*) break_ptr;
    pthread_mutex_init(&arena_ptr->mutex, NULL);
    pthread_mutex_lock(&arena_ptr->mutex);
    arena_ptr->nxt_arena = NULL;
    arena_ptr->is_in_list = 0;
    return arena_ptr;
}

Arena* get_arena() {
    Arena* arena_ptr = arena_list;
    while(arena_ptr && pthread_mutex_trylock(&arena_ptr->mutex)) {
        arena_ptr = arena_ptr->nxt_arena;
    }
    if(arena_ptr == NULL) {
        arena_ptr = create_arena();
    }
    return arena_ptr;
}

void release_arena(Arena* arena_ptr) {
    if (!arena_ptr->is_in_list) {
        arena_ptr->is_in_list = 0;
        append_arena_list(arena_ptr);
    }

    pthread_mutex_unlock(&arena_ptr->mutex);
}

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
    spare_chunk->arena = chunk->arena;

    chunk->next = spare_chunk;
    chunk->size = ALIGN32(size);
}

void append_free_list(Chunk* chunk, size_t page_index, Arena* arena) {
    if (arena->free_list[page_index]) {
        arena->free_list[page_index]->next = chunk;
        chunk->next = NULL;
        chunk->prev = arena->free_list[page_index];
        arena->free_list[page_index] = arena->free_list[page_index]->next;
    } else {
        arena->free_list[page_index] = chunk;
        arena->free_list[page_index]->prev = NULL;
        arena->free_list[page_index]->next = NULL;
    }
}

Chunk* alloc_memory(size_t size, Arena* arena) {
    size_t allocated_size = PAGE_ALIGN(size);
    pthread_mutex_lock(&mmap_lock);
    void* block = mmap(NULL, allocated_size, PROT_WRITE | PROT_READ,
                       MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    pthread_mutex_unlock(&mmap_lock);
    if (block == MAP_FAILED) {
        return NULL;
    }
    Chunk* new_block = block;
    new_block->size = allocated_size - METADATA_SIZE;
    new_block->next = NULL;
    new_block->arena = arena;
    if (new_block->size >= (BIN_ALIGN(size) << 1)) {
        split_block(new_block, size);
        append_free_list(new_block->next, PAGE_INDEX(size), arena);
    }
    
    return new_block;
}

Chunk* find_block(size_t size, Arena* arena) {
    Chunk* ptr = arena->free_list[PAGE_INDEX(size)];
    while (ptr != NULL) {
        if (WIDE_ENOUGH(ptr, size)) {
            break;
        }
        ptr = ptr->next;
    }
    return ptr;
}

void* pbmalloc_thread(size_t size, Arena* arena) {
    size_t page_index = PAGE_INDEX(size);
    Chunk* curr = NULL;

    if (arena->free_list[page_index]) {
        if (page_index != NUM_BINS - 1) {
            curr = arena->free_list[page_index];
            assert(arena->free_list[page_index] != NULL);
            assert(arena == arena->free_list[page_index]->arena);
            arena->free_list[page_index] = arena->free_list[page_index]->prev;
            curr->prev = NULL;
            curr->next = NULL;
            if (curr->size >= (BIN_ALIGN(size) << 1)) {
                split_block(curr, size);
                append_free_list(curr->next, page_index, arena);
            }
        } else {
            curr = find_block(size, arena);
            if (curr == NULL) {
                curr = alloc_memory(size, arena);
                if (curr == NULL)
                    return NULL;
            }
        }

    } else {
        curr = alloc_memory(size, arena);
        if (curr == NULL) {
            return NULL;
        }
    }
    return (void*) curr + METADATA_SIZE;
}

void* pbmalloc(size_t size) {
    Arena* arena = get_arena();
    void* ret = pbmalloc_thread(size, arena);
    release_arena(arena);
    return ret;
}

void pbfree(void* ptr) {
    if (ptr == NULL)
        return;
    void* metadata_ptr = ptr - METADATA_SIZE;
    Chunk* chunk_ptr = (Chunk*) metadata_ptr;
    if (chunk_ptr->size == 0)
        return;
    pthread_mutex_lock(&chunk_ptr->arena->mutex);
    append_free_list(chunk_ptr, PAGE_INDEX(chunk_ptr->size), chunk_ptr->arena);
    pthread_mutex_unlock(&chunk_ptr->arena->mutex);
}
