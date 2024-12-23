#include "helpers.h"
#include "debug.h"
#include "icsmm.h"

extern void * error_epi;
extern void * error_pro;
/* Helper function definitions go here */
int readjust_size(size_t size){
    int header_size = 8;
    int footer_size = 8;
    int alloc_size = 1;
    int temp_size = size%16;
    if(temp_size == 0){
        temp_size = 16;
    }
    int amountToAdd = 16-temp_size;
    return header_size+footer_size+alloc_size+size+amountToAdd;
}

int findBucket(size_t size){
    for(int i = 0; i<=4;i++){
        if ((seg_buckets+i)->max_size>=(size-1)){ //if bucket is big enough for data
            ics_free_header* freeHead = (seg_buckets+i)->freelist_head;
            while(freeHead!=NULL){
                if(freeHead->header.block_size>=size-1){
                    return i;
                }
                freeHead = freeHead->next;
            }
        }
    }
    return -1;
}
int incrementBrk(int size){
    int page_size = 4096;
    int real_size = page_size-sizeof(ics_header)-sizeof(ics_footer)-sizeof(ics_footer)-sizeof(ics_free_header);
    int counter = 0;
    while((real_size*counter) <size-1){
        void * temp = ics_inc_brk();
        if(temp == ((void*)-1)){
            return -1;
        }
        counter++;
    }
    return counter*page_size;
}
int add_more_memory(int size){
    void *curbrk = ics_get_brk();
    int block_size = incrementBrk(size);
    if(block_size == -1){
        return -1;
    }
    int bucket = 4;
    setEpiPro(curbrk, block_size);
    int newSize = block_size-sizeof(ics_footer)-sizeof(ics_header);
    void * headerP = curbrk+sizeof(ics_footer);
    ics_header tempHeader = {newSize, HEADER_MAGIC};
    ics_free_header temp_freeHeader = {tempHeader, NULL, NULL};
    ics_footer temp_footer = {newSize, size, FOOTER_MAGIC};
    *((ics_free_header*)headerP) = temp_freeHeader;
    *((ics_footer*)(headerP+newSize-sizeof(ics_footer))) = temp_footer;
    
    (seg_buckets+bucket)->freelist_head = headerP;
    
    return bucket;
}
void* add_more_memory_special(int size){
    void *curbrk = ics_get_brk();
    int block_size = incrementBrk(size);
    if(block_size == -1){
        return NULL;
    }
    int bucket = 4;
    setEpi(curbrk, block_size);
    int newSize = block_size-sizeof(ics_header)+sizeof(ics_footer);
    void * headerP = curbrk-sizeof(ics_header); //writing over old epilogue
    ics_header tempHeader = {newSize, HEADER_MAGIC};
    ics_free_header temp_freeHeader = {tempHeader, NULL, NULL};
    ics_footer temp_footer = {newSize, newSize, FOOTER_MAGIC};
    *((ics_free_header*)headerP) = temp_freeHeader;
    *((ics_footer*)(headerP+newSize-sizeof(ics_footer))) = temp_footer;
    
    //(seg_buckets+bucket)->freelist_head = headerP;
    
    return headerP;
}

void *addBlock(int bucket, size_t size, int requested){//need to change prev pointers for next free block
    ics_free_header* freeHead = (seg_buckets+bucket)->freelist_head;
    int freeHead_size = freeHead->header.block_size;
    while(freeHead_size<(size-1)){
        freeHead = freeHead->next;
        if(freeHead == NULL){
            break;
        }
        freeHead_size = freeHead->header.block_size;
    }
       //need to splinter from this block              //last free block
    int newblock_size = freeHead_size - (size-1);
    if(newblock_size<32){
        size+=newblock_size;
        newblock_size = 0;
    }
    //adding new header
    if (newblock_size>0){
        popOff(freeHead);
        //new free block head
        void * newHeadPointer = (void*)freeHead + size-1;
        ics_header tempHeader = {newblock_size, HEADER_MAGIC};
        ics_free_header newHeader = {tempHeader, NULL,NULL};
        *((ics_free_header*)newHeadPointer) = newHeader;
        //adding footer to allocated block
        void * newFooterPointer = (void*)freeHead+size-1-sizeof(ics_footer);
        ics_footer newFooter = {size, requested, FOOTER_MAGIC};
        *((ics_footer*)newFooterPointer) = newFooter;
        //update old footer
        ics_footer * oldFooterPointer = (ics_footer*)((void*)freeHead + freeHead_size - sizeof(ics_footer));
        oldFooterPointer->block_size = newblock_size;
        //update old header to size
        freeHead->header.block_size = size;
        //removing old block off freelist bucket
        int splitBucket = findFreeBucket(((ics_free_header*)newHeadPointer)->header.block_size);
        void * next = (seg_buckets+splitBucket)->freelist_head;
        (seg_buckets+splitBucket)->freelist_head = newHeadPointer;
        (seg_buckets+splitBucket)->freelist_head->next = next;
        if((seg_buckets+splitBucket)->freelist_head->next != NULL){
            (seg_buckets+splitBucket)->freelist_head->next->prev = (seg_buckets+splitBucket)->freelist_head;
        }
        return &(freeHead->next);
    }
    if(newblock_size == 0){ //
        void * newFooterPointer = (void*)freeHead+size-1-sizeof(ics_footer);
        ics_footer newFooter = {size, requested, FOOTER_MAGIC};
        *((ics_footer*)newFooterPointer) = newFooter;
        freeHead->header.block_size = size;
        popOff(freeHead);
        return &(freeHead->next);
    }
    return NULL;
}

void setEpiPro(void* startP, int size){
    ics_footer prologue = {1,0};
    ics_header epilogue = {1};
    *((ics_footer*)startP) = prologue;
    *((ics_header*)(startP+size-sizeof(ics_header))) = epilogue;
}
void setEpi(void* startP, int size){
    ics_header epilogue = {1};
    *((ics_header*)(startP+size-sizeof(ics_header))) = epilogue;
}


void placeInBucket(void *headerPointer, int bucket){
    ics_header header = *((ics_header*)headerPointer);
    ics_free_header* next = (seg_buckets+bucket)->freelist_head;
    ics_free_header freeHeader = {header, next, NULL};
    *((ics_free_header*)headerPointer) = freeHeader;
    if(next!= NULL){
        next->prev = headerPointer;
    }
    (seg_buckets+bucket)->freelist_head = headerPointer;
}

int findFreeBucket(uint64_t size){
    for(int i = 0; i<=4;i++){
        if ((seg_buckets+i)->max_size>=(size-1)){ //if bucket is big enough for data
            return i;
        }
    }
    return -1;
}

void * coalesce(void * frontFooter, void* midHeader, void* lastHeader){
    popOff(midHeader);
    void * headPointer = frontFooter+sizeof(frontFooter)-(((ics_footer*)frontFooter)->block_size);
    int alloc_bit_front = ((ics_footer*)frontFooter)->block_size & 1;
    int alloc_bit_back = 1;
    if(lastHeader != NULL){
        alloc_bit_back = ((ics_free_header*)lastHeader)->header.block_size&1;
    }
    if(alloc_bit_front == 1){
        headPointer = midHeader;
    }else{
        popOff(headPointer);
        ((ics_free_header*)headPointer)->header.block_size += ((ics_free_header*)midHeader)->header.block_size;
        ((ics_footer*)(midHeader+(((ics_free_header*)midHeader)->header.block_size)-sizeof(ics_footer)))->block_size = ((ics_free_header*)headPointer)->header.block_size;
    }
    if(alloc_bit_back == 1){
        return headPointer;
    }else{
        popOff(lastHeader);
        ((ics_free_header*)headPointer)->header.block_size += ((ics_free_header*)lastHeader)->header.block_size;
        ((ics_footer*)(lastHeader+(((ics_free_header*)lastHeader)->header.block_size)-sizeof(ics_footer)))->block_size = ((ics_free_header*)headPointer)->header.block_size;
        return headPointer;
    }
    return headPointer;
}

void popOff(void * ptr){
    for(int i = 0; i<=4;i++){
        ics_free_header* free_header = (seg_buckets+i)->freelist_head;
        int count = 0;
        while(free_header!=NULL){
            if(free_header == ptr){
                if (count ==0){
                    (seg_buckets+i)->freelist_head = free_header->next;
                    if((seg_buckets+i)->freelist_head!=NULL){
                        (seg_buckets+i)->freelist_head->prev = NULL;
                    }
                }else{
                    //free_header->next = ;
                    //free_header->prev = ;
                    free_header->prev->next = free_header->next;
                    if(free_header->next!=NULL){
                        free_header->next->prev = free_header->prev;
                    }
                }
                return;
            }
            free_header = free_header->next;
            count++;
        }
    }
}

int free_check(void * ptr){
    ics_header * headerPointer = ptr-sizeof(ics_header);
    ics_footer * footerPointer = ptr+(headerPointer->block_size-1)-sizeof(ics_footer)-sizeof(ics_header);
    if((headerPointer->block_size & 1) == 0){
        return -1;
    }
    if((footerPointer->block_size&1) == 0){
        return -1;
    }
    if(headerPointer->hid!=HEADER_MAGIC){
        return -1;
    }
    if(footerPointer->fid!=FOOTER_MAGIC){
        return -1;
    }
    if(headerPointer->block_size!=footerPointer->block_size){
        return -1;
    }
    if(ptr<error_pro || ptr>error_epi){
        return -1;
    }
    return 1;
}