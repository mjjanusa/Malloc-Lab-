/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "mjjanusacnegboh",
    /* First member's full name */
    "Michael Janusa",
    /* First member's email address */
    "mjjanusa@yahoo.com",
    /* Second member's full name (leave blank if none) */
    "Chinedu Egboh",
    /* Second member's email address (leave blank if none) */
    "cnegboh@cs.utexas.edu"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
static char *heap_listp = 0;	//points to the prologue block or first block

///////////////////////////////////////////////////////////////
/* Basic constants and macros */
 #define WSIZE 4 /* Word and header/footer size (bytes) */
 #define DSIZE 8 /* Double word size (bytes) */
 #define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

 #define MAX(x, y) ((x) > (y)? (x) : (y))

 /* Pack a size and allocated bit into a word */
 #define PACK(size, alloc) ((size) | (alloc))

 /* Read and write a word at address p */
 #define GET(p) (*(unsigned int *)(p))
 #define PUT(p, val) (*(unsigned int *)(p) = (val))

 /* Read the size and allocated fields from address p */
 #define GET_SIZE(p) (GET(p) & ~0x7)
 #define GET_ALLOC(p) (GET(p) & 0x1)

 /* Given block ptr bp, compute address of its header and footer */
 #define HDRP(bp) ((char *)(bp) - WSIZE)
 #define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

 /* Given block ptr bp, compute address of next and previous blocks */
 #define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
 #define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
///////////////////////////////////////////////////////////////

/* 
 * mm_init - initialize the malloc package.
 * mm init: Before calling mm malloc mm realloc or mm free, the application program (i.e.,
 * the trace-driven driver program that you will use to evaluate your implementation) calls mm init to
 * perform any necessary initializations, such as allocating the initial heap area. The return value should
 * be -1 if there was a problem in performing the initialization, 0 otherwise.
 */
int mm_init(void)
{
	//char *bp;
	/* Create the initial empty heap */
	if ((heap_listp = mem_sbrk(88*WSIZE)) == (void *)-1)
		return -1;
	
	PUT(heap_listp, 0); /* Alignment padding */

	PUT(heap_listp + (1*WSIZE), PACK(43*DSIZE, 1)); /* Prologue header */
	
	int i;
	for(i = 2; i < 86; i++) {
		PUT(heap_listp + (i*WSIZE), 0); /* initialize free pointers (one for every increment of 50 bytes*/
	}

	PUT(heap_listp + (86*WSIZE), PACK(43*DSIZE, 1)); /* Prologue footer */
	PUT(heap_listp + (87*WSIZE), PACK(0, 1)); /* Epilogue header */
	heap_listp += (2*WSIZE);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

    	return 0; 
}

////////////////////////////////////////////////////////////////
 static void *extend_heap(size_t words)
 {
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
	PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

	add_free_list(bp);

	/* Coalesce if the previous block was free */
	return coalesce(bp);
	//return bp;
 }
////////////////////////////////////////////////////////////////
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
   /* int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
		return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }*/

	////////////////////////////////////////////////////////////
	size_t asize; /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	char *bp;

	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize,CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, asize);
	return bp;

	////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////

 static void *find_fit(size_t asize)
 {
 	/* First fit search */
 	void *bp;
 	
 	int minlist = asize / 50;
 	if(minlist > 83)
 		minlist = 83; 
 	for(; minlist < 84; minlist++){
		for (bp = (char *)GET(heap_listp + (minlist * WSIZE)); (int)bp != 0 && GET_SIZE(HDRP(bp)) > 0; bp = (char *)GET(bp+WSIZE)) {
			if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
				return bp;
			}
		}
 	}
 	return NULL; /* No fit */
 }

 static void place(void *bp, size_t asize)
 {
 	void *nxt_bp;
 	size_t csize = GET_SIZE(HDRP(bp));

 	if ((csize - asize) >= (2*DSIZE)) {
 		
 		//REMOVE bp form free list
 		remove_free_list(bp);
 		
 		PUT(HDRP(bp), PACK(asize, 1));
 		PUT(FTRP(bp), PACK(asize, 1));
 		nxt_bp = NEXT_BLKP(bp);
 		PUT(HDRP(nxt_bp), PACK(csize-asize, 0));
 		PUT(FTRP(nxt_bp), PACK(csize-asize, 0));
 		 		
 		//ADD nxt_bp to free list
 		add_free_list(nxt_bp);
 	}
 	else {
 		PUT(HDRP(bp), PACK(csize, 1));
 		PUT(FTRP(bp), PACK(csize, 1));
 		
 		remove_free_list(bp);
 	}
 }
 
 
 static void remove_free_list(void *bp)
 {	
 	int minlist; 
 	int size;
 	
 	size = GET_SIZE(HDRP(bp));
 	
 	minlist = size / 50;
 	if(minlist > 83)
 		minlist = 83; 
	if(GET(bp) == 0 && GET(bp + WSIZE) == 0) // if the prev free pointer and next free pointer were 0 set global first free pointer to 0.
 		PUT(heap_listp+(minlist * WSIZE), 0); 	
 	else if (GET(bp) == 0 && GET(bp + WSIZE) != 0){// else if the prev pointer was 0 and next not zero make global first free pointer next.
 		PUT(heap_listp+(minlist * WSIZE), GET(bp + WSIZE));
 		PUT((char *)GET(bp + WSIZE), 0);
 	}
 	else if (GET(bp) != 0 && GET(bp + WSIZE) == 0) // if prev pointer not 0 and next 0 then make prev's next pointer 0.
 		PUT(((char *)GET(bp) + WSIZE), 0);
 	else {//if prev pointer and next pointer not 0 update pointers 
 		PUT(((char *)GET(bp) + WSIZE), GET(bp + WSIZE));	
 		PUT(((char *)GET(bp + WSIZE)), GET(bp));	
 	}
 }
 
 static void add_free_list(void *bp)
 {	 
 	int minlist;
 	void *temp_next;
 	int size;
 	
 	size = GET_SIZE(HDRP(bp));
 	minlist = size / 50;
	if(minlist > 83)
		minlist = 83; 
	temp_next = (char *)GET(heap_listp + (minlist * WSIZE)); // get global next. 
	PUT(heap_listp + (minlist * WSIZE), (int)bp); //set global first free block to this block.

	if((int)temp_next != 0) // if the old global next was not 0, update the old global next's previous free block pointer to this block.
		PUT(temp_next, (int)bp);

	PUT(bp, 0); 
	PUT(bp+WSIZE, (int)temp_next);
 }

////////////////////////////////////////////////////////////////////


/*
 * mm_free - Freeing a block does nothing.
 */
//void mm_free(void *ptr)
void mm_free(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));

	//add bp to free list
	add_free_list(bp);

	coalesce(bp);
}
////////////////////////////////////////////////////////////////
 static void *coalesce(void *bp)
 {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) { /* Case 1 */
		return bp;
	}

	else if (prev_alloc && !next_alloc) { /* Case 2 */

 		//REMOVE BP FROM FREE LIST
 		remove_free_list(bp);
 		//REMOVE NEXT FROM FREE LIST
 		remove_free_list(NEXT_BLKP(bp));

 		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));

		//ADD BP TO THE FREE LIST
		add_free_list(bp);


	}

	else if (!prev_alloc && next_alloc) { /* Case 3 */


		//REMOVE BP FROM FREE LIST
 		remove_free_list(bp);
 		//REMOVE PREV FROM FREE LIST
 		remove_free_list(PREV_BLKP(bp));

		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);

		//ADD TO THE FREE LIST
		add_free_list(bp);

	}

	else { /* Case 4 */
		
		//REMOVE BP FROM FREE LIST
 		remove_free_list(bp);
 		//REMOVE PREV FROM FREE LIST
 		remove_free_list(PREV_BLKP(bp));
 		//REMOVE NEXT FROM FREE LIST
 		remove_free_list(NEXT_BLKP(bp));
		
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
		GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		
		//ADD TO THE FREE LIST
		add_free_list(bp);
		
	}
	return bp;
 }
////////////////////////////////////////////////////////////////
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)	
      return NULL;
    if (oldptr == NULL)	//if ptr is NULL, the call is equivalent to mm malloc(size)
	return newptr;
    if (size == 0){	//if size is equal to zero, the call is equivalent to mm free(ptr)
	mm_free(ptr);
    	newptr = 0;
    }
    copySize = GET_SIZE(HDRP(ptr));
    //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
