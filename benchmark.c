#include "mmap-bins/pbmalloc.h"
#include <stdio.h>
#include <time.h>

#define NUM_ROUNDS 100000
#define ARRAY_SIZE 1000
#define STRIDE 7

void test() {
    int* v[NUM_ROUNDS];
    for(size_t i = 0; i < NUM_ROUNDS; i++) {
        v[i] = malloc(ARRAY_SIZE * sizeof(int));
    }

    for(size_t i = 0; i < NUM_ROUNDS; i++) {
        for(size_t j = 0; j < ARRAY_SIZE; j++)
            v[i][j] = rand();
    }

    for(size_t i = 0; i < NUM_ROUNDS; i+=STRIDE)
        pbfree(v[i]);

    for(size_t i = 0; i < NUM_ROUNDS; i += 7) {
        v[i] = malloc(ARRAY_SIZE * sizeof(int));
    }
}

int main() {
    clock_t begin = clock();
    test();
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;\
    printf("%f", time_spent);
}