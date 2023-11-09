#include "umem.h"
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

static int allocAlgo = 0; //Which alllocation algorithm was selected on memory initilization
static void* headOfFree = NULL; //Pointer to the start of free space
static void* headofMemSpace = NULL;
static int memInit = 0; //Whether memory has already been initilized

//Will represent a node in our linked list
//of memory chunks inside the space
//allocated after umeminit call
struct memChunk {
    void* address; //start address of chunk
    int isFree; //Flag (0) is free (1) is allocated
    memChunk* next; //pointer to next chunk
};

/**TODO
Add basic linked lsit features to break up
the VM space allocated by umeminit into
a linked list of memChunks
*/

int umeminit(size_t sizeOfRegion, int allocationAlgo){

    if(sizeOfRegion <= 0 || memInit == true){
        //if map fail, if memory is already initilized, or size is <= 0 return -1
        return -1;
    }
    else{
        int fd = open("/dev/zero", O_RDWR);

        size_t page_size = getpagesize();

        //Region is a multiple of page size
        size_t total_size = sizeOfRegion + (page_size - (sizeOfRegion % page_size));
        printf("%ld\n",total_size);

        // sizeOfRegion (in bytes) needs to be evenly divisible by the page size
        if( (headOfFree = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0))== MAP_FAILED){
            return -1;
        }

        headofMemSpace = headOfFree;

        for(int i = 0; i<(sizeOfRegion / page_size); i++){
            //build linked list
        }

        printf("%p",headOfFree);
        allocAlgo = allocationAlgo;
        memInit = true;
        close(fd);
        return 0;
    }
}

void *umalloc(size_t size){
    //search through linked list depending
    //on allocation method definied on
    //initial call to umeminit
}

//int ufree(void *ptr){
//
//}
//
//void umemdump(){
//
//}
