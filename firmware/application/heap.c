#include <FreeRTOS.h>
#include <semphr.h>

#include <stdlib.h>

#include "heap.h"

extern unsigned char _sheap, _eheap;

static void heapLock(void *arg);
static void heapUnlock(void *arg);

static const heap_locking_callbacks_t heapLockTable = {
    .lock = heapLock,
    .unlock = heapUnlock
};

static void heapLock(void *arg) {
    xSemaphoreTake((xSemaphoreHandle) arg, portMAX_DELAY);
}

static void heapUnlock(void *arg) {
    xSemaphoreGive((xSemaphoreHandle) arg);
}

int initializeHeapLocks(void) {
    xSemaphoreHandle heapMutex = xSemaphoreCreateMutex();
    if(heapMutex == NULL)
        return -1;

    heap_initialize_locking(&heapLockTable, heapMutex);

    return 0;
}

void initializeHeap(void) {    
    heap_seed(&_sheap, &_eheap - &_sheap); 
}