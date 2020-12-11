#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include "stddef.h"
#include "string.h"

#define MAX_SIZE 1048576 // 1 Mb - Total size

char* head;
size_t free_size;
char* max_addr; // maximum possible address
char* min_addr; // minimum possible address
typedef struct mem1_block{
    uint8_t isn_free;
    struct mem1_block* prev;
    struct mem1_block* next;
    size_t size;
} mem_block;

// Merge free block into the block1
int merge_right(mem_block* block1){
    mem_block* block2;
    mem_block* prev;
    mem_block* next;
    size_t size, newsize, oldsize;
    uint8_t isn_free = 0;
    size_t* sptr;
    uint8_t* uptr;
    char* ptr = (char*)block1;

    size = block1->size;
    block2 = (mem_block*)(ptr + size);
    newsize = size + block2->size;

    if ((block2->isn_free)||((char*)block2 >= max_addr))
        return 0;

    block1->size = newsize;

    if(block2->prev != NULL)
        block2->prev->next = block2->next;

    if((block2->next != NULL))
        block2->next->prev = block2->prev;

    uptr = (uint8_t*)(ptr + newsize - sizeof(uint8_t));
    memcpy(uptr, &isn_free, sizeof(uint8_t));
    sptr = (size_t*)(ptr + newsize - sizeof(uint8_t) - sizeof(size_t));
    memcpy(sptr, &newsize, sizeof(size_t));

    //clear old info
    memset(ptr + size, 0, sizeof(mem_block));
    memset(ptr + size - sizeof(uint8_t) - sizeof(size_t), 0, sizeof(size_t) + sizeof(uint8_t));
    
    return 1;
}

//  Call this function before using myalloc to initialize memory allocator
void mysetup(void *buf, size_t size){
    mem_block mem_b;
    size_t* sptr;
    uint8_t* uptr;
    uint8_t isn_free = 0;

    head = (char*)buf;
    max_addr = (char*)((char*)buf + size);
    min_addr = (char*)buf;

    mem_b.isn_free = 0;
    mem_b.prev = NULL;
    mem_b.next = NULL;
    mem_b.size = size; 

    uptr = (uint8_t*)((char*)head + size - sizeof(uint8_t));
    sptr = (size_t*)(head + size + sizeof(uint8_t) + sizeof(size_t));
    memcpy(uptr, &isn_free, sizeof(uint8_t));
    memcpy(sptr, &mem_b.size, sizeof(size_t));
    memcpy(head, &mem_b, sizeof(mem_block));
    
    free_size = size - sizeof(mem_block) - sizeof(size_t) - sizeof(uint8_t);
}

void *myalloc(size_t size){
    if((size > MAX_SIZE) || (size == 0) || (free_size < size))
        return NULL;

    mem_block  *tmp;
    mem_block  tmp2;
    mem_block *newb;
    mem_block *next_b;
    tmp = (mem_block*)(head);
    size_t newsize;
    size_t* sptr;
    uint8_t* uptr;

    uint cnt = 0;

    do{
        if ((size + (sizeof(uint8_t) + sizeof(mem_block) + sizeof(size_t))*2 < tmp->size) && (tmp->isn_free == 0)){
            uint8_t isn_free = 1;

            tmp2.size = size + sizeof(mem_block) + sizeof(size_t) + sizeof(uint8_t);
            tmp2.prev = NULL;
            tmp2.next = NULL;
            
            tmp2.isn_free = 1;

            char* ptr = (char*)tmp;
            size_t oldsize = tmp->size;

            memcpy(ptr + oldsize - tmp2.size, &tmp2, sizeof(mem_block)); // create new block

            uptr = (uint8_t*)(ptr + oldsize - sizeof(uint8_t) - sizeof(size_t));
            sptr = (size_t*)(ptr + oldsize - sizeof(uint8_t));

            memcpy(sptr, &isn_free, sizeof(uint8_t)); // isn_free byte setting
            memcpy(uptr, &tmp2.size, sizeof(size_t));
            memcpy(ptr + oldsize - tmp2.size, &tmp2, sizeof(mem_block)); // create new block

            newb = (mem_block*)(ptr + oldsize - tmp2.size);
            newsize = oldsize - newb->size;

            tmp->size = newsize;
            isn_free = 0;

            uptr = (uint8_t*)(ptr + newsize - sizeof(uint8_t));
            sptr = (size_t*)(ptr + newsize - sizeof(uint8_t) - sizeof(size_t));

            memcpy(uptr, &isn_free, sizeof(uint8_t)); // isn_free byte setting
            memcpy(sptr, &newsize, sizeof(size_t)); // size bytes setting

            free_size -= newb->size;

            return (ptr + newsize + sizeof(mem_block));
        } else if(tmp->size == (size + (sizeof(uint8_t) + sizeof(size_t) + sizeof(mem_block))) && (tmp->isn_free == 0)){
            size_t sizee = tmp->size;
            char* ptr = (char*)tmp;
            tmp->isn_free = 1;

            uint8_t isn_free = 1;
            uptr = (uint8_t*)(ptr + sizee - sizeof(uint8_t));
            sptr = (size_t*)(ptr + sizee - sizeof(uint8_t) - sizeof(size_t));
            memcpy(uptr, &isn_free, sizeof(uint8_t));
            memcpy(sptr, &sizee, sizeof(size_t));
            if(tmp->next)
                head = (char*)tmp->next;
            return (ptr + sizeof(mem_block));
        } else if(tmp->next) { 
            tmp = tmp->next; // to the next free block
            cnt += 1;
            if (tmp == tmp->next || tmp == tmp->next->next)
                return NULL;
            continue;
        } else
            return NULL;
	} while(tmp != NULL);

	return NULL; // if there is no block with enough size
}   

void myfree(void *p){
    mem_block *pm_block;
    mem_block *right_block;
    mem_block *left_block = NULL;
    mem_block *tmp = (mem_block*)(head);
    uint8_t isn_free = 0;
    size_t size;
    size_t* sptr;
    uint8_t* uptr;

    uptr = (uint8_t*)((char*)pm_block - sizeof(uint8_t));

    if (p == NULL)
        return;

    char* pc = (char*)p;
    pm_block = (mem_block*)(pc - sizeof(mem_block));

    if(!(pm_block->isn_free)){
        p = NULL;
        return;
    }

    size = pm_block->size;
    pm_block->isn_free = 0;
    uptr = (uint8_t*)((char*)pm_block + size - sizeof(uint8_t));
    memcpy(uptr, &isn_free, sizeof(uint8_t));

    free_size += pm_block->size;
    // check right neighboor
    right_block = (mem_block*)((char*)pm_block + size);

    if(!merge_right(pm_block)){
        pm_block->next = tmp;
        tmp->prev = pm_block;
    }

    char* ptr = (char*)pm_block;

    sptr = (size_t*)(ptr - sizeof(uint8_t) - sizeof(size_t));
    uptr = (uint8_t*)((char*)ptr - sizeof(uint8_t));
    uint8_t uval = (uint8_t)*(uptr);

    if(!(uval) && (ptr - sizeof(size_t) - sizeof(uint8_t)) >= min_addr){
        size_t leftsize = (size_t)*(sptr);

        if(!leftsize)
            goto _out;

        left_block = (mem_block*)(uptr + sizeof(uint8_t) - leftsize);
        right_block = pm_block;

        if(merge_right(left_block)){
        } else {
            left_block->next = tmp;
            tmp->prev = left_block;
        }

        left_block->prev = NULL;
        head = (char*)left_block;

        goto _out;
    }
    
    pm_block->prev = NULL;
    head = (char*)pm_block;

_out:
    p = NULL;
}
