
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef char *addrs_t; // define addrs_t as a char pointer
typedef void *any_t; // pointer to an address of a generic type

void VInit (size_t size);
addrs_t Malloc (size_t size);
static void place(void *bp, size_t asize);
static void *find_first_fit (size_t asize);
static void *coalesce (void *bp);
addrs_t *VMalloc(size_t size);
void VFree(addrs_t *addr);
addrs_t *VPut(any_t data, size_t size);
void VGet(any_t return_data, addrs_t *addr, size_t size);
int find_index_of_first_free_in_RT();

#define R 1<<16
/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */	
#define DSIZE       8			/* Doubleword size (bytes) */
#define DEFAULT_MEM_SIZE (1<<20)  /* Default size of memory */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))	

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))	
#define PUT(p, val)  (*(unsigned int *)(p) = (val))	

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)	// include the size of both footer and header
#define GET_ALLOC(p)	(GET(p) & 0x1) 

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)	
#define FTRP(bp)			 ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))	
#define PREV_BLKP(bp)	 ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* $end mallocmacros */


static addrs_t heap_listp = 0;
addrs_t RT[R];

void VInit (size_t size) {

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
      return NULL; 
    /* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)			
		asize = 2 * DSIZE;		
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);	

	/* Search the free list for a fit */
	if ((bp = find_first_fit (asize)) != NULL) {
		place (bp, asize);		
		return bp;
	}
  else
  {
    return NULL;
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
{   size_t RTindex = find_index_of_first_free_in_RT();
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= (2*DSIZE)) { 
	    PUT(HDRP(bp), PACK(asize, 1));
	    PUT(FTRP(bp), RTindex); // place index in the end of block
      RT[RTindex]=bp; 
	    bp = NEXT_BLKP(bp);
	    PUT(HDRP(bp), PACK(csize-asize, 0));
	    PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else { 
	    PUT(HDRP(bp), PACK(csize, 1));
	    PUT(FTRP(bp), RTindex); // place index in the end of block
      RT[RTindex]=bp; 
    }
}
/* $end mmplace */

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce (void *bp)
{ // only case of next block free due to compaction
	size_t size = GET_SIZE (HDRP (bp));
		size += GET_SIZE (HDRP (NEXT_BLKP (bp)));
		PUT (HDRP (bp), PACK (size, 0));
		PUT (FTRP (NEXT_BLKP (bp)), PACK (size, 0));
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
    if (!GET_ALLOC (HDRP (bp)) && (asize <= GET_SIZE (HDRP (bp)))) 
    {
      return bp;
		}
	}
	return NULL;				/* No fit */
}

/*
* find_index_of_first_free_in_RT - Find the first free spot in RT from 0 to R
*/
int find_index_of_first_free_in_RT()
{ // find index of first free in table
  int i =0;
  while(RT[i]!=NULL&&i<R)
  {
    i++;
  }
  return i; 
}


static void compact(addrs_t bp)
{
int next_alloc = GET_ALLOC (HDRP(NEXT_BLKP(bp)));
/* Case 1: The next block is free. There's no need for compaction */
if (!next_alloc)
{
  size_t size = GET_SIZE(HDRP(bp)); 
  int index = GET(FTRP(bp)); // get index in footer

  if (index<R) 
    RT[index]= NULL; // free in table
  
  PUT(HDRP(bp),PACK(size,0)); 
  PUT(FTRP(bp),PACK(size,0));
  coalesce(bp);

}
/*Case 2: The next block is not free. We have to compact the rest of the blocks */
else if (next_alloc)
{
  int incr = GET_SIZE(HDRP(bp)); //Get the size of the block we freed
  int index = GET(FTRP(bp));
  RT[index]= NULL;
  addrs_t next_block = NEXT_BLKP(bp); //address of the next block to move up
  size_t snext = GET_SIZE(HDRP(next_block));
  
  while (GET_ALLOC(HDRP(next_block))&&snext!=0) // if next block is free or epilougue footer is hit, stop the loop
  {
    memmove(HDRP(bp), HDRP(next_block), snext); // shift block one by one
    index = GET(FTRP(bp)); 
    RT[index]= bp; // get index and change it to the right address
    bp = bp+snext; // move pointer to the next space to put the block into
    next_block = bp + incr; // move current_block to the next block we need to move
    snext = GET_SIZE(HDRP(next_block)); // get size of block 
  }
  PUT(HDRP(bp),PACK(incr,0)); // create the header and footer for the free block
  PUT(FTRP(bp),0);
  PUT(FTRP(bp),PACK(incr,0));
  coalesce(bp); //coalesce the two free space in the end
  
  }
}

void VFree(addrs_t* addr)
{
  if (*addr==0)
    return;
  addrs_t a = *addr; 
  size_t size = GET_SIZE(HDRP(a));
  compact(a);
}
addrs_t *VMalloc(size_t size)
{
  addrs_t bp = Malloc(size); // call malloc to allocate space. get pointer
  if (bp==NULL)
    return NULL;
  int index = GET(FTRP(bp)); 
  return &RT[index]; // return the address of the element in RT
}

addrs_t *VPut(any_t data, size_t size)
{
  addrs_t *RTpointer = VMalloc(size);
  
  if (RTpointer == NULL) //No space available on the heap
    return NULL; 
  addrs_t bp = *RTpointer; 
  memmove(bp,data,size); 
  return RTpointer;
}
void VGet(any_t return_data, addrs_t* addr, size_t size)
{
  addrs_t bp = *addr; 
  int block_size = GET_SIZE(HDRP(bp));
  if (size<=block_size)
    {
      int counter = 0;
      memmove(return_data, bp, size);
      VFree(addr);
    }
}

