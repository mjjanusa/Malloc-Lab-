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
 * PROGRESS!
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
////////////////Function Declaration////////////////////////////

static char *heap_listp = NULL;		//points to the prologue block or first block
int global_minlist = 0;  			//GLOBAL FIRST LIST WITH FREE BLOCKS.
int free_count = 0;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void add_free_list(void *bp);
static void remove_free_list(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static int mm_check(void);

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
	global_minlist = 100;
	free_count = 0;

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
 	
 	if(free_count == 0)
 		return NULL;
 	
 	int minlist = asize / 50;
 	if(minlist > 83)
 		minlist = 83; 
 	if(minlist < global_minlist)
 		minlist = global_minlist;
 	for(; minlist < 84; minlist++){
 		int i = 0;
		for (bp = (char *)GET(heap_listp + (minlist * WSIZE)); (int)bp != 0 && GET_SIZE(HDRP(bp)) > 0 && i < 250; bp = (char *)GET(bp+WSIZE)) {
			if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
				return bp;
			}
			i++;
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
 	free_count--; 
 	
 	size = GET_SIZE(HDRP(bp));
 	
 	minlist = size / 50;
 	if(minlist > 83)
 		minlist = 83; 
	if(GET(bp) == 0 && GET(bp + WSIZE) == 0) { // if the prev free pointer and next free pointer were 0 set global first free pointer to 0.
 		PUT(heap_listp+(minlist * WSIZE), 0);
 		if(global_minlist == minlist) { //if this list was the global min list update global minlist.
 			if(free_count > 0){
 			int i;
 			for (i = minlist; GET(heap_listp+(i * WSIZE)) == 0; i++);
 			global_minlist = i;
 			}
			else(global_minlist = 100); 			
 		}
 	}
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
 	void *temp_cur;
 	void *temp_prev;
 	int size;
 	
 	free_count++;
 	
 	size = GET_SIZE(HDRP(bp));
 	minlist = size / 50;
	if(minlist > 83)
		minlist = 83;
	if(global_minlist > minlist || global_minlist == 100)
		global_minlist = minlist; //update global min list
	temp_cur = (char *)GET(heap_listp + (minlist * WSIZE));
	if(temp_cur == 0){
		PUT(heap_listp + (minlist * WSIZE), (int)bp);	
		PUT(bp, 0); 
		PUT(bp+WSIZE, 0);
	}
	else {
		temp_prev = (char *)GET(heap_listp + (minlist * WSIZE));
		for (; (int)temp_cur != 0 && GET_SIZE(HDRP(temp_cur)) < size; temp_cur = (char *)GET(temp_cur+WSIZE))
			temp_prev = temp_cur;

		temp_cur = temp_prev;
		temp_next = (char *)GET(temp_cur + WSIZE); // get global next. 
		PUT(temp_cur + WSIZE, (int)bp); //set global first free block to this block.

		if((int)temp_next != 0) // if the old global next was not 0, update the old global next's previous free block pointer to this block.
		PUT(temp_next, (int)bp);

		PUT(bp, (int)temp_cur); 
		PUT(bp+WSIZE, (int)temp_next);
		}
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

	coalesce(bp);
}
////////////////////////////////////////////////////////////////
 static void *coalesce(void *bp)
 {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) { // Case 1 
		//ADD BP TO THE FREE LIST
		add_free_list(bp);
		
		return bp;
	}

	else if (prev_alloc && !next_alloc) { // Case 2 

 		//REMOVE NEXT FROM FREE LIST
 		remove_free_list(NEXT_BLKP(bp));

 		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));

		//ADD BP TO THE FREE LIST
		add_free_list(bp);


	}

	else if (!prev_alloc && next_alloc) { // Case 3 

 		//REMOVE PREV FROM FREE LIST
 		remove_free_list(PREV_BLKP(bp));

		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);

		//ADD TO THE FREE LIST
		add_free_list(bp);

	}

	else { // Case 4 

 		//REMOVE PREV FROM FREE LIST
 		remove_free_list(PREV_BLKP(bp));
 		//REMOVE NEXT FROM FREE LIST
 		remove_free_list(NEXT_BLKP(bp));

		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
		GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
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
    void *temp;
    size_t tempsize;
    size_t copySize;
    
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(oldptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(oldptr)));
    size_t size_prev = GET_SIZE(HDRP(oldptr));
    int increase;
    if(size_prev  < size + DSIZE) 
    	    increase = 1;
    else
    	    increase = 0;
    
    
    if (size == 0){	//if size is equal to zero, the call is equivalent to mm free(ptr)
	mm_free(ptr);
    	newptr = 0;
    	return NULL;
    }
    if (oldptr == NULL)	//if ptr is NULL, the call is equivalent to mm malloc(size)
	return mm_malloc(size);    
    if(increase == 0 && (size_prev - size - DSIZE) > (2*DSIZE)){ // if shrinking ptr and released space will be large enough to be a block
    	if (size <= DSIZE)
    	    	size = 2*DSIZE;
    	 else
	  	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // align size
	if((size_prev - size) > (2*DSIZE)){

	 assert ( size <= size_prev);
    	 PUT(HDRP(oldptr), PACK(size, 1)); // resize old 
	 PUT(FTRP(oldptr), PACK(size, 1)); // resize old
	 newptr = oldptr; // set new ptr to old ptr
	 oldptr =  (NEXT_BLKP(newptr)); // reset old pointer to the new (empty) block
	 PUT(HDRP(oldptr), PACK(size_prev - size, 0));
	 PUT(FTRP(oldptr), PACK(size_prev - size, 0));
	 assert(GET_SIZE(HDRP(newptr)) +GET_SIZE(HDRP(oldptr)) == size_prev);  
	//add oldptr to free list
	add_free_list(oldptr);
	//coalesce
	coalesce(oldptr);
	return oldptr;
	}
    }
    if(increase == 0) {//if shrinking ptr and released space to small to be a block or size is the same return same ptr
    	    return ptr;
    }
    else { // size is greater than before
    	    //if next is unallocated and combining next with this block would fufill new size requirement merge the blocks
    	    if(next_alloc == 0 && ((GET_SIZE(HDRP(NEXT_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){
    	    	    temp = NEXT_BLKP(oldptr);
    	    	    tempsize = GET_SIZE(FTRP(temp));
    	    	    //remove next block from the free list
    	    	    remove_free_list(NEXT_BLKP(ptr));
    	    	    if (size <= DSIZE)
    	    	    	size = 2*DSIZE;
		    else
		    	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // align size
    	    	    if((tempsize + size_prev) < (size + 2*DSIZE)) // if not big enough for new free block make new block take all space
    	    	    	 size = tempsize + size_prev;
	    	    assert(tempsize + size_prev >= size);
    	    	    PUT(HDRP(oldptr), PACK(size, 1)); // resize old 
    	    	    PUT(FTRP(oldptr), PACK(size, 1)); // resize old 
    	    	    assert(GET_SIZE(HDRP(oldptr)) == size);
    	    	    
    	    	    if((tempsize + size_prev) >= (size + 2*DSIZE)){ //if new free block initialize it
			    newptr = NEXT_BLKP(oldptr); // set new pointer to the new (empty) block
			    PUT(HDRP(newptr), PACK(tempsize + size_prev - size, 0));
			    PUT(FTRP(newptr), PACK(tempsize + size_prev - size, 0));
			    //add newptr to free list
			    add_free_list(newptr);
    	    	    }
    	    	    //assert(GET_SIZE(HDRP(oldptr)) + GET_SIZE(HDRP(newptr)) == tempsize + size_prev);
    	    	    return oldptr;    	    	    
    	    }    	
    	    //if prev is unallocated and combining prev with this block would fufill new size requirement merge the blocks
    	    else if(prev_alloc == 0 && ((GET_SIZE(HDRP(PREV_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){ 
    	    	    newptr = PREV_BLKP(oldptr);
    	    	    tempsize = GET_SIZE(FTRP(newptr));
    	    	    //remove next block from the free list
    	    	    remove_free_list(PREV_BLKP(oldptr));
    	    	    if (size <= DSIZE)
    	    	    	size = 2*DSIZE;
		    else
		    	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // align size
    	    	    if((tempsize + size_prev) < (size + 2*DSIZE)) // if not big enough for new free block make new block take all space
    	    	    	 size = tempsize + size_prev;
	    	    assert(tempsize + size_prev >= size);
    	    	    PUT(HDRP(newptr), PACK(size, 1)); // resize old
    	    	    copySize = GET_SIZE(HDRP(oldptr));
    	    	    memcpy(newptr, oldptr, copySize);
    	    	    PUT(FTRP(newptr), PACK(size, 1)); // resize old 
    	    	    assert(GET_SIZE(HDRP(newptr)) == size);
    	    	    
    	    	    if((tempsize + size_prev) >= (size + 2*DSIZE)){ //if new free block initialize it
			    temp = NEXT_BLKP(newptr); // set new pointer to the new (empty) block
			    PUT(HDRP(temp), PACK(tempsize + size_prev - size, 0));
			    PUT(FTRP(temp), PACK(tempsize + size_prev - size, 0));
			    //add newptr to free list
			    add_free_list(temp);
    	    	    }
    	    	    //assert(GET_SIZE(HDRP(oldptr)) + GET_SIZE(HDRP(newptr)) == tempsize + size_prev);
    	    	    return newptr;    	
    	    }
    	    //if next and prev are unallocated and combining the three blocks would fufill new size requirement merge blocks.
    	    else if (next_alloc == 0 && prev_alloc == 0 && ((GET_SIZE(HDRP(PREV_BLKP(oldptr)))) + (GET_SIZE(HDRP(NEXT_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){
    	   	    newptr = PREV_BLKP(oldptr);
    	   	    temp = NEXT_BLKP(oldptr);
    	    	    tempsize = GET_SIZE(FTRP(newptr)) + GET_SIZE(FTRP(temp));
    	    	    //remove next block from the free list
    	    	    remove_free_list(PREV_BLKP(oldptr));
    	    	    remove_free_list(NEXT_BLKP(oldptr));
    	    	    if (size <= DSIZE)
    	    	    	size = 2*DSIZE;
		    else
		    	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // align size
    	    	    if((tempsize + size_prev) < (size + 2*DSIZE)) // if not big enough for new free block make new block take all space
    	    	    	 size = tempsize + size_prev;
	    	    assert(tempsize + size_prev >= size);
    	    	    PUT(HDRP(newptr), PACK(size, 1)); // resize old
    	    	    copySize = GET_SIZE(HDRP(oldptr));
    	    	    memcpy(newptr, oldptr, copySize);
    	    	    PUT(FTRP(newptr), PACK(size, 1)); // resize old 
    	    	    assert(GET_SIZE(HDRP(newptr)) == size);
    	    	    
    	    	    if((tempsize + size_prev) >= (size + 2*DSIZE)){ //if new free block initialize it
			    temp = NEXT_BLKP(newptr); // set new pointer to the new (empty) block
			    PUT(HDRP(temp), PACK(tempsize + size_prev - size, 0));
			    PUT(FTRP(temp), PACK(tempsize + size_prev - size, 0));
			    //add newptr to free list
			    add_free_list(temp);
    	    	    }
    	    	    //assert(GET_SIZE(HDRP(oldptr)) + GET_SIZE(HDRP(newptr)) == tempsize + size_prev);
    	    	    return newptr;    	   	    	    
    	    }
    	    // if next and previous blocks are allocated
    	    newptr = mm_malloc(size);
    	    copySize = GET_SIZE(HDRP(oldptr));
	    if (size < copySize)
		    copySize = size;
	    //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
	    memcpy(newptr, oldptr, copySize);
	    mm_free(oldptr);
	    return newptr;
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////
/*
 *  Your heap checker will consist of the function int mm check(void) in mm.c. It will check any invariants
 *  or consistency conditions you consider prudent. It returns a nonzero value if and only if your heap is
 *  consistent. You are not limited to the listed suggestions nor are you required to check all of them. You are
 *  encouraged to print out error messages when mm check fails. 
 */
 static int mm_check(void)
 {
	//char *tempPtr;
	void* tempPtr;
	size_t* topHeap =  mem_heap_lo();	//Get top of heap from mdriver.c
    	size_t* bottomHeap =  mem_heap_hi();	//And get footer of heap

  	//While there is a next pointer, go through the heap
	for(tempPtr = topHeap; GET_SIZE(HDRP(tempPtr)) > 0; tempPtr = NEXT_BLKP(tempPtr)) {
	//for(tempPtr = GET_BP(heap_listp); GET_SIZE(HDRP(tempPtr)) > 0; tempPtr = NEXT_BLKP(tempPtr)) {
        	
		//If pointer is beyond bounds print error
        	if (tempPtr > bottomHeap || tempPtr < topHeap){	
            		printf("Error: pointer %p out of heap bounds\n", tempPtr);
		}

		//if the size and allocated fields read from address p are "0" print error 
		//(contiguous free block issue)
        	if (GET_ALLOC(tempPtr) == 0 && GET_ALLOC(NEXT_BLKP(tempPtr))==0){
            		printf("Error: Empty stacked blocks %p and %p not coalesced\n", tempPtr, NEXT_BLKP(tempPtr));
		}
		/* header/footer consistency */
		if (GET(HDRP(tempPtr)) != GET(FTRP(tempPtr))){
        		//printf("Error, %p head and bottom of block not consistent\n", tempPtr);
			printf("Block %x: Header %d, Footer %d\n", (size_t)tempPtr, GET(HDRP(tempPtr)), GET(FTRP(tempPtr)));
    		}
		if ((size_t)tempPtr%8){
        		printf("Error, %p misaligned our headers and payload\n", tempPtr);
    		}
	}
	return 0;

/*
    void* current;
    void* previous;
    int i;
    int safety_counter = 0;
 
    // header/footer consistency 
    current = GET_BP(heap_listp);
    do {
        if (GET(HDRP(current)) != GET(FTRP(current))) {
            printf("Header/footer mismatch:\n");
            printf("Block %x: Header %d, Footer %d\n", (size_t)current, GET(HDRP(current)), GET(FTRP(current)));
            return 0;
        }
        current = NEXT_BLKP(current);
    } while (GET_SIZE(HDRP(current)) > 0);

 
    return 1;*/
 }
//////////////////////////////////////////////////////////////////////////////////////////////
