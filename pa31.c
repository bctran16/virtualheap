// Your code for part 1 goes here.
// Do not include your main function

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef char *addrs_t; // define addrs_t as a char pointer
typedef void *any_t; // pointer to an address of a generic type

void Init (size_t size);
addrs_t Malloc (size_t size);
void Free (addrs_t addr);
addrs_t Put (any_t data, size_t size);
void Get (any_t return_data, addrs_t addr, size_t size);
static void place(void *bp, size_t asize);
static void *find_first_fit (size_t asize);
static void *coalesce (void *bp);


/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8            /* Doubleword size (bytes) */
#define DEFAULT_MEM_SIZE (1<<20)  /* Default size of memory */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)    // include the size of both footer and header
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)             ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)     ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* $end mallocmacros */


static addrs_t heap_listp = 0;  

void Init (size_t size) {

   addrs_t baseptr;

   /* Use the system malloc() routine (or new in C++) only to allocate size bytes for
      the initial memory area, M1. baseptr is the starting address of M1. */
   baseptr = (addrs_t) malloc (size);

  // Initialization of the heap

   PUT(baseptr, 0);                            // Alignment padding
   PUT(baseptr + (1*WSIZE), PACK(DSIZE,1));    // Prologue header
   PUT(baseptr + (2*WSIZE), PACK(DSIZE,1));    // Prologue footer
   PUT(baseptr + (size-WSIZE), PACK(0,1));     // Epilogue header
   PUT(baseptr + (3*WSIZE), PACK(size-2*DSIZE,0));  // Free block header after heap created
   PUT(baseptr + (size-2*WSIZE), PACK(size-2*DSIZE,0)); // Free block footer after heap
   baseptr +=(2*WSIZE);
   heap_listp = baseptr;
   
}

addrs_t Malloc (size_t size) {

  //  Implement your own memory allocation routine here.
  //  This should allocate the first contiguous size bytes available in M1.
  //   Since some machine architectures are 64-bit, it should be safe to allocate space starting
  //   at the first address divisible by 8. Hence align all addresses on 8-byte boundaries!

  //   If enough space exists, allocate space and return the base address of the memory.
  //   If insufficient space exists, return NULL.
    size_t asize;      /* Adjusted block size */
    char *bp;

    if (size==0)
    {
      return NULL;
    }
      
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_first_fit (asize)) != NULL) {
        place (bp, asize); //allocating and coalescing blocks
        return bp;
    }
  else
  {
    return NULL;
  }
}

void Free (addrs_t addr) {

    // This frees the previously allocated size bytes starting from address addr in the
    // memory area, M1. You can assume the size argument is stored in a data structure after
    // the Malloc() routine has been called, just as with the UNIX free() command.
    size_t size = GET_SIZE(HDRP(addr));  //get the size of the block that is free
    PUT(HDRP(addr), PACK(size, 0)); //Free the block by header
    PUT(FTRP(addr), PACK(size, 0)); // ree the block by header
    coalesce(addr);
    
}

addrs_t Put (any_t data, size_t size) {

  //  Allocate size bytes from M1 using Malloc().
  //  Copy size bytes of data into Malloc'd memory.
  //  You can assume data is a storage area outside M1.
  //  Return starting address of data in Malloc'd memory.
  addrs_t bp = Malloc(size);
  if (bp!=NULL)
  {
    memmove(bp, data, size); // move data to bp
    return bp;
  }
  
}

void Get (any_t return_data, addrs_t addr, size_t size) {

    // Copy size bytes from addr in the memory area, M1, to data address.
    // As with Put(), you can assume data is a storage area outside M1.
    // De-allocate size bytes of memory starting from addr using Free().
  int counter = 0;
  if(size<=GET_SIZE(HDRP(addr)))
  {
    memmove(return_data, addr, size); //move data from addr to return_data
    Free(addr);
  }

}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize)
     /* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2*DSIZE)) { // if size of blick is larger than 16 split
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0)); // splitting the rest to free
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else { // else just pack
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
/* $end mmplace */

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce (void *bp)
{
    size_t prev_alloc = GET_ALLOC (FTRP (PREV_BLKP (bp)));
    size_t next_alloc = GET_ALLOC (HDRP (NEXT_BLKP (bp)));
    size_t size = GET_SIZE (HDRP (bp));

    if (prev_alloc && next_alloc) {    /* Case 1: no need to coalesce*/
        return bp;
    }

    else if (prev_alloc && !next_alloc) {    /* Case 2: next block free */
        size += GET_SIZE (HDRP (NEXT_BLKP (bp)));
        PUT (HDRP (bp), PACK (size, 0));
        PUT (FTRP (NEXT_BLKP (bp)), PACK (size, 0));
    }

    else if (!prev_alloc && next_alloc) {    /* Case 3: previous block free */
        size += GET_SIZE (HDRP (PREV_BLKP (bp)));
        PUT (FTRP (bp), PACK (size, 0));
        PUT (HDRP (PREV_BLKP (bp)), PACK (size, 0));
        bp = PREV_BLKP (bp);
    }

    else {                        /* Case 4: both previous and next block free */
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
      GET_SIZE(FTRP(NEXT_BLKP(bp)));
      PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
      PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
      bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * find_first_fit - Find a fit for a block with asize bytes
 */
static void * find_first_fit (size_t asize)
{
    /* First fit search */
    void *bp;

    for (bp = heap_listp; GET_SIZE (HDRP (bp)) > 0; bp = NEXT_BLKP (bp)) {
  
        if (!GET_ALLOC (HDRP (bp)) && (asize <= GET_SIZE (HDRP (bp)))) {
          return bp;
        }
    }
    return NULL;                /* No fit */
}

