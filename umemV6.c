#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

static int allocAlgo = 0; //Allocation method selected either (1,2,3,4)
static void* headOfFree = NULL; //Keeps track of where to start search for Next_Fit
static int memInit = 0; //Flag memory slab has already been allocated

/*
Represents either a unallocated or allocated block of memory
*/
struct memChunk {
    int isFree;
    size_t size;
    struct memChunk* next;
};
struct memChunk* head = NULL;

/*
SizeOfRegion: Requested size for memory slab (will added onto till nearest page size)
allocationAlgo: Request algorithm to be used by umalloc(1,2,3,4)

Intilizes a slab of memory requested by the user, will be used to store
our linked list of allocated and unallocated memory blocks

Return: (-1) Failed to allocate slab (0) Allocation successful
*/
int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    //If requested size is to small or a slab
    //has already been intilized error
    if (sizeOfRegion <= 0 || memInit == true) {
        return -1;
    } else {
        int fd = open("/dev/zero", O_RDWR);
        size_t page_size = getpagesize();
        //total_size is equal to requested size rounded up to the OS nearest page size
        size_t total_size = sizeOfRegion + (page_size - (sizeOfRegion % page_size));

        //Request slab of memory
        if ((headOfFree = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
            close(fd);
            return -1;
        }

        //create the first node which is our unallocated block
        head = (struct memChunk*)headOfFree;
        head->isFree = 0;
        head->next = NULL;
        head->size = total_size;

        //Set algorithm for future and flag that a slab has been allocated
        allocAlgo = allocationAlgo;
        memInit = true;

        close(fd);
        return 0;
    }
}

/*
cur: Is the current node being read by an allocation algorithm
     will be used to split into to node one being the allocated block
     and the other being the new free block
size: requested size made in umalloc

splits a existing block in our linked list into an
allocated section and a unallocated section

Return: Start of allocated space excluding header
*/
void* split(struct memChunk* cur, size_t size){
    //rounds up the requested size to the nearest multiple of 8
    //min requested block is of size 8 bytes
    size_t adjustedSize = size + (8 - (size % 8)) % 8;

    //new free block will have an address of (cur address + header(24 bytes) + round requested size)
    struct memChunk* newBlock = (struct memChunk*)((char*)cur + sizeof(struct memChunk) + adjustedSize);

    //set header values of free block
    newBlock->isFree = 0;
    newBlock->next = cur->next;
    newBlock->size = cur->size - sizeof(struct memChunk) - adjustedSize;

    //set header values of allocated block
    cur->next = newBlock;
    cur->size = adjustedSize;
    cur->isFree = 1;

    //If we are using next fit set the new
    //free block as the next search start
    if(allocAlgo == 4){
        headOfFree = newBlock;
    }
    //returns address of the start of requested block(moves past header)
    return (void*)((char*)cur+sizeof(struct memChunk));
}

/*
size: requested size made in umalloc

Uses best fit algorithm to find available memory block

Return: Start of allocated space excluding header
*/
void* Best_Fit(size_t size){
    //Error if requested size is less than 1 byte
    if(size < 1){
        return NULL;
    }

    struct memChunk* cur = head; //iterator through linked list
    int sizeSmallest = 0; //smallest size that fits requested size
    int tempSmallest = 0; //holds temp smallest while iterating
    int minimum = 8; //minium size allowed for allocation
    int noFit = 1; //Flag (1) no chunk of correct size (0) space avaliable

    //find best fit inside linked list
    while(cur != NULL){
        //current size is large enough and is free
        if(cur->size >= size && cur->isFree == 0 && cur->size >= minimum){
            //First fit found update smallest
            if(noFit == 1){
                tempSmallest = cur->size;
                noFit = 0; //fit found
            }
            //New smallest
            if(tempSmallest > cur->size){
                tempSmallest= cur->size;
            }
        }
        cur = cur->next;
    }

    //no avaliable space
    if(noFit == 1){
        return NULL;
    }

    sizeSmallest = tempSmallest;
    cur = head; //Reset head

    //search for found smallest and either split or update
    while(cur != NULL){
        if(cur->size == sizeSmallest){
            //split block
            if(sizeSmallest > size){
                return split(cur, size);
            }
            //smallest size is the size requested
            cur->isFree = 1;
            //returns start of memChunk
            return (void*)((char*)cur+sizeof(struct memChunk));
        }
        cur = cur->next;
    }

    //Should never be reached
    return NULL;
}

/*
size: requested size made in umalloc

Uses worst fit algorithm to find available memory block

Return: Start of allocated space excluding header
*/
void* Worst_Fit(size_t size){

    if(size < 1){
        return NULL;
    }

    struct memChunk* cur = head; //iterator through linked list
    int sizeLargest = 0; //largest size that fits requested size
    int tempLargest = 0; //holds temp largest while iterating
    int minimum = 8; //minium size allowed for allocation
    int noFit = 1; //Flag (1) no chunk of correct size (0) space avaliable

    //find worst fit inside linked list
    while(cur != NULL){
        //enough space for allocation
        if(cur->size >= size && cur->isFree == 0 && cur->size >= minimum){
            //First fit found set tempLargest
            if(noFit == 1){
                tempLargest = cur->size;
                noFit = 0; //fit found
            }
            //update largest
            if(tempLargest < cur->size){
                tempLargest= cur->size;
            }
        }
        cur = cur->next;
    }

    //no avaliable space
    if(noFit == 1){
        return NULL;
    }

    sizeLargest = tempLargest;
    cur = head; //reset iterator

    //search for found largest and either split or update
    while(cur != NULL){
        if(cur->size == sizeLargest){
            //split block
            if(sizeLargest > size){
                return split(cur, size);
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

/*
size: requested size made in umalloc

Uses first fit algorithm to find available memory block

Return: Start of allocated space excluding header
*/
void* First_Fit(size_t size){

    struct memChunk* cur = head; //Iterator through linked list

    //find first large enough and free chunk
    while(cur != NULL){
        //split block
        if(cur->size > size && cur->isFree == 0){
            return split(cur, size);
        }
        else if(cur->size == size && cur->isFree == 0){
            return (void*)((char*)cur+sizeof(struct memChunk));
        }
        cur = cur->next;
    }

    //Block of correct size not found
    return NULL;
}

/*
size: requested size made in umalloc

Uses next fit algorithm to find available memory block

Return: Start of allocated space excluding header
*/
void* Next_Fit(size_t size){

    struct memChunk* cur = headOfFree;

    //search from last spot
    while(cur != NULL){
        //split block
        if(cur->size > size && cur->isFree == 0){
            return split(cur,size);
        }
        else if(cur->size == size && cur->isFree == 0){
            return (void*)((char*)cur+sizeof(struct memChunk));
        }
        cur = cur->next;
    }

    //if none found search from beginning till last head of free address
    cur = head;
    while(cur != headOfFree){
        //split block
        if(cur->size > size && cur->isFree == 0){
            return split(cur,size);
        }
        else if(cur->size == size && cur->isFree == 0){
            headOfFree = cur->next;
            return (void*)((char*)cur+sizeof(struct memChunk));
        }
        cur = cur->next;
    }

    //Block of correct size not found
    return NULL;
}

/*
size: requested size

Sends memory request to the selected allgorithm made during slab allocation

Returns: address to the start of requested memory block
*/
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
    case 4:
        return Next_Fit(size);
    }

    //Algorithm not found
    return NULL;
}

/*
Caolesces any consecutive free space inside the slab into a larger block
*/
void Coalesce(){
    struct memChunk* cur = head; //Iterator through linked list

    //goes through and add free blocks togther till the end of the linked list
    while (cur != NULL && cur->next != NULL) {
        if ((cur->isFree == 0 && cur->next->isFree == 0)) {
            //add size of coalesced block plus the header
            cur->size += cur->next->size + sizeof(struct memChunk);
            cur->next = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

/*
ptr: is the address of the block to be freed

frees a block of allocated memory and does any necessary coalescing after the free

Returns: (0) on successful free (-1) on failed free
*/
int ufree(void *ptr) {
    struct memChunk* cur = head;

    if(ptr == NULL)
        return -1;

    while (cur != NULL) {
        if ((void*)cur == (ptr - sizeof(struct memChunk))) {
            cur->isFree = 0;
            Coalesce();
            return 0;
        }
        cur = cur->next;
    }
    //failed to free chunk
    return -1;
}


void umemdump(){
    struct memChunk* cur = head;

    while(cur != NULL){
        if(cur->isFree == 0){
            printf("SIZE: %ld  ADDRESS: %p  FREE: %d\n", cur->size, cur, cur->isFree);
        }
        cur = cur->next;
    }
    printf("\n");
}
