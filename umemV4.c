#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

static int allocAlgo = 0;
static void* headOfFree = NULL;
static int memInit = 0;

struct memChunk {
    int isFree;
    size_t size;
    struct memChunk* prev; //8 bytes
    struct memChunk* next; //8 bytes
};

struct memChunk* head = NULL;

void printMemoryBlock(){
    struct memChunk* cur = head;
    while(cur != NULL){
        printf("\n\nCurrent Size: %ld Free: %d\n\n", cur->size ,cur->isFree);
        cur = cur->next;
    }
}

int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    if (sizeOfRegion <= 0 || memInit == true) {
        return -1;
    } else {
        int fd = open("/dev/zero", O_RDWR);
        size_t page_size = getpagesize();
        size_t total_size = sizeOfRegion + (page_size - (sizeOfRegion % page_size));

        if ((headOfFree = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
            return -1;
        }

        head = (struct memChunk*)headOfFree;
        head->isFree = 0;
        head->prev = NULL;
        head->next = NULL;
        head->size = total_size;

        allocAlgo = allocationAlgo;
        memInit = true;

        close(fd);
        return 0;
    }
}


void* Best_Fit(size_t size){

    if(size < 1){
        return NULL;
    }

    struct memChunk* cur = head;
    int sizeSmallest = 0;
    int tempSmallest = 0;

    //find best
    int noFit = 1; //Flag (1) no chunk of correct size (0) space avaliable

    while(cur != NULL){
        //enough space for allocation
        printf("\nCurrent Size: %ld Free: %d", cur->size, cur->isFree);
        if(cur->size >= size && cur->isFree == 0){
            noFit = 0; //fit found
            tempSmallest = cur->size;
            if(tempSmallest > cur->size){
                //update smallest as long as
                //its greater than or equal to 8 bytes
                if(cur->size >= 8){
                    tempSmallest= cur->size;
                    printf(" curretn smallest: %d", tempSmallest);
                }
            }
        }
        cur = cur->next;
    }

    //no avaliable space
    if(noFit == 1){
        return NULL;
    }
    sizeSmallest = tempSmallest;
    printf("\nSMALLEST: %d\n", sizeSmallest);
    cur = head;
    while(cur != NULL){
        if(cur->size == sizeSmallest){
            //split block
            if(sizeSmallest > size){
                size_t adjustedSize = size + (8 - (size % 8)) % 8;
                struct memChunk* newBlock = (struct memChunk*)((char*)cur + sizeof(struct memChunk) + adjustedSize);

                newBlock->isFree = 0;
                newBlock->next = cur->next;
                newBlock->prev = cur;
                newBlock->size = cur->size - sizeof(struct memChunk) - adjustedSize;
                printf(" New Free: %ld",newBlock->size);
                cur->next = newBlock;
                cur->size = adjustedSize;
            }

            cur->isFree = 1;
            //returns start of memChunk
            return (void*)((char*)cur+sizeof(struct memChunk));
        }
        cur = cur->next;
    }

    //Should never be reached
    return NULL;
}

void* Worst_Fit(size_t size){

    if(size < 1){
        return NULL;
    }

    struct memChunk* cur = head;
    int sizeLargest = 0;
    int tempLargest = cur->size;

    //find best
    int noFit = 1; //Flag (1) no chunk of correct size (0) space avaliable

    while(cur != NULL){
        //enough space for allocation
        printf("\nCurrent Size: %ld Free: %d", cur->size, cur->isFree);
        if(cur->size >= size && cur->isFree == 0){
            noFit = 0; //fit found
            if(tempLargest < cur->size){
                //update smallest as long as
                //its greater than or equal to 8 bytes
                if(cur->size >= 8){
                    tempLargest= cur->size;
                    printf(" current largest: %d", tempLargest);
                }
            }
        }
        cur = cur->next;
    }

    //no avaliable space
    if(noFit == 1){
        return NULL;
    }
    sizeLargest = tempLargest;
    printf("\nLARGEST: %d\n", sizeLargest);
    cur = head;
    while(cur != NULL){
        if(cur->size == sizeLargest){
            //split block
            if(sizeLargest > size){
                size_t adjustedSize = size + (8 - (size % 8)) % 8;
                struct memChunk* newBlock = (struct memChunk*)((char*)cur + sizeof(struct memChunk) + adjustedSize);

                newBlock->isFree = 0;
                newBlock->next = cur->next;
                newBlock->prev = cur;
                newBlock->size = cur->size - sizeof(struct memChunk) - adjustedSize;
                printf(" New Free: %ld",newBlock->size);
                cur->next = newBlock;
                cur->size = adjustedSize;
            }

            cur->isFree = 1;
            //returns start of memChunk
            return (void*)((char*)cur+sizeof(struct memChunk));
        }
        cur = cur->next;
    }

    //Should never be reached
    return NULL;
}

void* First_Fit(size_t size){

    struct memChunk* cur = head;

    while(cur != NULL){
        //split block
        if(cur->size > size){
            size_t adjustedSize = size + (8 - (size % 8)) % 8;
            struct memChunk* newBlock = (struct memChunk*)((char*)cur + sizeof(struct memChunk) + adjustedSize);

            newBlock->isFree = 0;
            newBlock->next = cur->next;
            newBlock->prev = cur;
            newBlock->size = cur->size - sizeof(struct memChunk) - adjustedSize;
            printf(" New Free: %ld",newBlock->size);

            cur->next = newBlock;
            cur->size = adjustedSize;
            cur->isFree = 1;
            //returns start of memChunk
            return (void*)((char*)cur+sizeof(struct memChunk));
        }
        cur = cur->next;
    }

    //Block of correct size not found
    return NULL;
}

//returns NULL if error allocating
//either not enough space or trying
//to allocate something with size < 1
void* umalloc(size_t size) {

    switch(allocAlgo){

    //Best Fit
    case 1:
        return Best_Fit(size);

    //Worst Fit
    case 2:
        return Worst_Fit(size);

    //First Fit
    case 3:
        return First_Fit(size);

    //Next Fit
    }


    return NULL;
}

void Coalesce(){
    struct memChunk* cur = head;

    while (cur != NULL && cur->next != NULL) {
        if ((cur->isFree == 0 && cur->next->isFree == 0)) {
            cur->size += cur->next->size;
            cur->next = cur->next->next;
        } else {
            cur = cur->next;
        }
    }

    // Check if the last block needs coalescing
    if (cur != NULL && cur->next == NULL && cur->isFree == 0) {
        cur->size += sizeof(struct memChunk);
        cur->next = NULL;
    }
}

int ufree(void *ptr) {
    struct memChunk* cur = head;

    while (cur != NULL) {
        if ((void*)cur == (ptr - sizeof(struct memChunk))) {
            printf("Freeing block: %p\n", cur);
            cur->isFree = 0;
            Coalesce();

            printf("Block freed successfully\n");
            return 0;
        }
        cur = cur->next;
    }
    //failed to free chunk
    printf("Failed to free block: %p\n", ptr);
    return -1;
}


//void umemdump(){
//
//}
