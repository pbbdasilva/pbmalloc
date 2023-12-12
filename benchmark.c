#include "pbmalloc.h"
#include <stdio.h>
#include <time.h>

#define NUM_ROUNDS 100000
#define ARRAY_SIZE 1000
#define STRIDE 7

clock_t begin, end;

void test_pbmalloc() {
    int* v[NUM_ROUNDS];
    size_t size[NUM_ROUNDS];

    begin = clock();
    for(size_t i = 0; i < NUM_ROUNDS; i++) {
        size_t v_size = (rand() % ARRAY_SIZE);
        v[i] = pbmalloc(v_size * sizeof(int));
        size[i] = v_size;
    }

    for(size_t i = 0; i < NUM_ROUNDS; i++) {
        for(size_t j = 0; j < size[i]; j++)
            v[i][j] = rand();
    }

    for(size_t i = 0; i < NUM_ROUNDS; i+=STRIDE)
        pbfree(v[i]);

    for(size_t i = 0; i < NUM_ROUNDS; i += STRIDE) {
        size_t v_size = (rand() % ARRAY_SIZE);
        v[i] = pbmalloc(v_size * sizeof(int));
        size[i] = v_size;
    }

    for(size_t i = 0; i < NUM_ROUNDS; i++)
        pbfree(v[i]);

    end = clock();
}

void test_malloc() {
    int* v[NUM_ROUNDS];
    size_t size[NUM_ROUNDS];

    begin = clock();
    for(size_t i = 0; i < NUM_ROUNDS; i++) {
        size_t v_size = (rand() % ARRAY_SIZE);
        v[i] = malloc(v_size * sizeof(int));
        size[i] = v_size;
    }

    for(size_t i = 0; i < NUM_ROUNDS; i++) {
        for(size_t j = 0; j < size[i]; j++)
            v[i][j] = rand();
    }

    for(size_t i = 0; i < NUM_ROUNDS; i+=STRIDE)
        free(v[i]);

    for(size_t i = 0; i < NUM_ROUNDS; i += STRIDE) {
        size_t v_size = (rand() % ARRAY_SIZE);
        v[i] = malloc(v_size * sizeof(int));
        size[i] = v_size;
    }

    for(size_t i = 0; i < NUM_ROUNDS; i++)
        free(v[i]);

    end = clock();
}

int main() {
//    test_malloc();
//    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//    printf("%f\n", time_spent);
    test_pbmalloc();
//    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//    printf("%f\n", time_spent);
}