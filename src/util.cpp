#include "util.h"
#include "stdlib.h"

// crudely estimate free heap by comparing the pointers of a heap and stack variable
int estimateFreeHeap(int allocSize) {
    uint8_t stackVar;
    uint8_t *heapVar = (uint8_t *)malloc(allocSize);
    if (heapVar == NULL) {
        return 0;
    }
    int freeHeap = &stackVar - heapVar;
    free(heapVar);
    return freeHeap;
}

