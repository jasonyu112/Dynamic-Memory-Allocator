#include "icsmm.h"
#include "debug.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * This is your implementation of malloc. It acquires uninitialized memory from  
 * ics_inc_brk() that is 16-byte aligned, as needed.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return If successful, the pointer to a valid region of memory of at least the
 * requested size is returned. Otherwise, NULL is returned and errno is set to 
 * ENOMEM - representing failure to allocate space for the request.
 * 
 * If size is 0, then NULL is returned and errno is set to EINVAL - representing
 * an invalid request.
 */

// need to check for more than 6 increments
void * error_pro = NULL;
void * error_epi = NULL;
void *ics_malloc(size_t size) {
    if(size == 0){
        errno = EINVAL;
        return NULL;
    }
    int requested = size;
    size = readjust_size(size);
    int bucket = -1;
    void * newH = NULL;
    if((bucket = findBucket(size)) != -1){
        return addBlock(bucket, size, requested);
    }else{
        if((seg_buckets+4)->freelist_head==NULL&&(seg_buckets+3)->freelist_head==NULL&&(seg_buckets+2)->freelist_head==NULL&&(seg_buckets+1)->freelist_head==NULL&&seg_buckets->freelist_head==NULL){
            error_pro = ics_get_brk();
            bucket = add_more_memory(size);
            error_epi = ics_get_brk();
            if(bucket == -1){
                errno = ENOMEM;
                return NULL;
            }
        }else{
            ics_footer * freeFooter = ics_get_brk()-sizeof(ics_header)-sizeof(ics_footer);
            ics_free_header* freeHead = ((void*)freeFooter)+sizeof(ics_footer)-(freeFooter->block_size);
            void* middle = add_more_memory_special(size-1-(freeHead->header.block_size));
            error_epi = ics_get_brk();
            if(middle == NULL){
                errno = ENOMEM;
                return NULL;
            }
            freeHead = coalesce(middle-sizeof(ics_footer),middle, NULL);
            ics_free_header* next = (seg_buckets+4)->freelist_head;
            freeHead->next = next;
            if (freeHead->next!=NULL){
                freeHead->next->prev = freeHead;
            }
            (seg_buckets+4)->freelist_head = freeHead;
            bucket = 4;
        }
        return addBlock(bucket, size, requested);
    }

    return newH;
}

/*
 * Marks a dynamically allocated block as no longer in use and coalesces with 
 * adjacent free blocks. Adds the block to the appropriate bucket according
 * to the block placement policy.
 *
 * @param ptr Address of dynamically allocated memory returned by the function
 * ics_malloc.
 * 
 * @return 0 upon success, -1 if error and set errno accordingly.
 * 
 * If the address of the memory being freed is not valid, this function sets errno
 * to EINVAL. To determine if a ptr is not valid, (i) the header and footer are in
 * the managed  heap space, (ii) check the hid field of the ptr's header for
 * 0x100decafbee5 (iii) check the fid field of the ptr's footer for 0x0a011dab,
 * (iv) check that the block_size in the ptr's header and footer are equal, and (v) 
 * the allocated bit is set in both ptr's header and footer. 
 */
//need to check if valid pointer
int ics_free(void *ptr) {
    if(free_check(ptr) == -1){
        errno = EINVAL;
        return -1;
    }
    ics_header * headerPointer = ptr-sizeof(ics_header);
    if(headerPointer->block_size%2 ==1){
        headerPointer->block_size -=1;
    }
    headerPointer = coalesce(ptr-sizeof(ics_header)-sizeof(ics_footer),ptr-sizeof(ics_header),ptr-sizeof(ics_header)+(headerPointer->block_size));
    ics_footer * footerPointer = ptr+(headerPointer->block_size)-sizeof(ics_footer)-sizeof(ics_header);
    if (footerPointer->block_size%2 ==1){
        footerPointer->block_size -=1;
    }
    int bucket = findFreeBucket(headerPointer->block_size);
    placeInBucket(headerPointer, bucket);

    return 0;
}

/*
 * EXTRA CREDIT!!!
 *
 * Resizes the dynamically allocated memory, pointed to by ptr, to at least size 
 * bytes. Before, attempting to realloc, the current block is coalesced with the free
 * adjacent block with the higher address only.  
 * 
 * If the current block will exactly satisfy the request (without creation of a 
 * splinter), return the current block. 
 * 
 * If reallocation to a larger size, the current (possibly coalesced) block will be
 * used if large enough to satisfy the request. If the current block is not large
 * enough to satisfy the request,  a new block is selected from the free list. All
 * payload bytes of the original block are copied to the new block, if the block was 
 * moved. 
 *
 * If the reallocation size is a reduction in space from the current (possible
 * coalesced) block, resize the block as necessary, creating a new free block if the 
 * free'd space will not produce a splinter.
 *
 * @param ptr Address of the previously allocated memory region.
 * @param size The minimum size to resize the allocated memory to.
 * @return If successful, the pointer to the block of allocated memory is
 * returned. Else, NULL is returned and errno is set appropriately.
 *
 * If there is no memory available ics_malloc will set errno to ENOMEM. 
 *
 * If ics_realloc is called with an invalid pointer, set errno to EINVAL. See ics_free
 * for more details.
 *
 * If ics_realloc is called with a valid pointer and a size of 0, the allocated     
 * block is free'd and return NULL.
 */
void *ics_realloc(void *ptr, size_t size) {
    return NULL;
}
