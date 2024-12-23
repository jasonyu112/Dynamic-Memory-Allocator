#ifndef HELPERS_H
#define HELPERS_H
#include <stdlib.h>
#include <stdint.h>
#include <icsmm.h>
#include <stdbool.h>

int readjust_size(size_t size);
int findBucket(size_t size);
int add_more_memory(int size);
void *addBlock(int bucket, size_t size, int requested);
void setEpiPro(void* startP, int size);
int incrementBrk(int size);
void placeInBucket(void *headerPointer, int bucket);
int findFreeBucket(uint64_t size);
void* add_more_memory_special(int size);
void * coalesce(void * frontFooter, void* midHeader, void* lastHeader);
void popOff(void * ptr);
void setEpi(void* startP, int size);
int free_check(void * ptr);

#endif
