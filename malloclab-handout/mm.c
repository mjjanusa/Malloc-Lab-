/*
 We used the standard header and footer from the book for allocated and free blocks.  Blocks are allocated back to back on the heap.  

Our free list uses an explicit free list.  Each free block uses the two words between the header and footer to hold a pointer to the next free block and the previous free block.

In addition to having an explicit free list we also split the free list into segregated fits.  We implemented this by adding 84 words to the prologue block.  Each of these words contains a pointer to the first block of that free list.
Each pointer represents a segmented fit of 50 bytes.  IE. The first pointer is for free blocks between 0 and 49 bytes, the second pointer is for free blocks between 50 and 99 bytes and so on.   
If there are no blocks in a given fit segment the pointer is 0.  Each free list is also maintained in order so smaller blocks are always before larger blocks.  This makes first fit also best fit.  

Free lists are accessed within functions by adding list number multiplied by wsize to the main heap_listp pointer.  

To optimize speed we also added two additional global vairables, global_minlist and free_count.  Free count is as it's name describes a count of the free blocks on the heap.  This is used to immediately know there are no free blocks without traversing the list of pointers..
global_minlist holds the list number for the free list that holds the first free block.  This allows the algorithm to skip all of the empty lists instantly.  global_minlist is set to 100 when there are no free blocks.  Note: it may seem repetitive to use global_minlist and free count,
but using both simplified the process of determining when the list is now empty, otherwise we would have to traverse the rest of the free lists before setting global_minlist to 100 signifying no free blocks.  
  
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

static char *heap_listp = NULL;		//points to the prologue block
int global_minlist = 0;  		//GLOBAL FIRST LIST WITH FREE BLOCKS
int free_count = 0;			//GLOBAL COUNT OF FREE BLOCKS
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

/* 
 * mm_init - initialize the malloc package.
 * mm init: Before calling mm malloc mm realloc or mm free, the application program (i.e.,
 * the trace-driven driver program that you will use to evaluate your implementation) calls mm init to
 * perform any necessary initializations, such as allocating the initial heap area. The return value should
 * be -1 if there was a problem in performing the initialization, 0 otherwise.
 *
 * Initializes 84 words in the prologue block as pointers for our array of free lists.  Sets all of the pointers to 0 signifying the free list is empty.  
 *
 *This function also initializes free_count to 0 and global_minlist to 100 signifying no free blocks.  
 *
 */
int mm_init(void)
{
	/* Create the initial empty heap */
	if ((heap_listp = mem_sbrk(88*WSIZE)) == (void *)-1)//create block for prologue and epilogue header. 
		return -1;

	PUT(heap_listp, 0); /* Alignment padding */

	PUT(heap_listp + (1*WSIZE), PACK(43*DSIZE, 1)); /* Prologue header */

	int i;
	for(i = 2; i < 86; i++) {
		PUT(heap_listp + (i*WSIZE), 0); /* initialize free pointers (one for every increment of 50 bytes*/
	}

	PUT(heap_listp + (86*WSIZE), PACK(43*DSIZE, 1)); /* Prologue footer */
	PUT(heap_listp + (87*WSIZE), PACK(0, 1)); /* Epilogue header */
	heap_listp += (2*WSIZE);// increase heap_listp to point to prologue. 
	global_minlist = 100; // initialize global_minlist to indicate no free blocks
	free_count = 0;  //set free count to 0 to signify no free blocks.  

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

    	return 0; 
}


/*Function to extend heap by size bytes.  Creates new free block at the end of list. */
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

	/* Coalesce to combine neighboring free blocks if possible */
	//return coalesce(bp);
	bp = coalesce(bp);
	//mm_check();
	return bp;
 }
 
 
/* 
 *   mm_malloc - Allocate a block by finding a large enough block on the free list (extending heap if needed) and placing it in the free block.   
 *   Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
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
		//if found place in block
		place(bp, asize);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize,CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	
	//place in new memory.  
	place(bp, asize);
	return bp;

}


/* 
 *      find_fit - search free list for large enough block to place malloc request.  Return null if no fit fount.  
 *  	find_fit starts search at the minimum free list that a large enough block could be on.  
 *	virtual best fit search due to free list organization.
 */
 static void *find_fit(size_t asize)
 {
 	/* First fit search */
 	void *bp;
 	
 	//if there are no free blocks, return null
 	if(free_count == 0)
 		return NULL;
 	
 	//calculate the minimum list number that a large enough block could be on
 	int minlist = asize / 50;
 	
 	//since there are only 83 lists if minimum list calculation results in list higher than 83 reset minlist to 83.
 	if(minlist > 83)
 		minlist = 83; 
 	
 	//if calculated min list is less than the global min list reset min list to global minlist because all free lists in between are empty. 
 	if(minlist < global_minlist)
 		minlist = global_minlist;
 	
 	//Loop through the free lists starting at min list.  
 	for(; minlist < 84; minlist++){
 		int i = 0;
 		//search through the current list for a free block big enough for this request.  if one is found this is the best fit so return bp. 
 		//also stops searching after it has looked at 250 elements of the list.  this is to prevent getting hung up on a really long list (probably of all the same sized elements).  Probably not the best solution to this issue, but effective and makes a judgement call of time over space allocation.
		for (bp = (char *)GET(heap_listp + (minlist * WSIZE)); (int)bp != 0 && GET_SIZE(HDRP(bp)) > 0 && i < 250; bp = (char *)GET(bp+WSIZE)) {
			if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
				return bp;
			}
			i++;
		}
 	}
 	//if no fits wer found return null
 	return NULL;
 }
 
/* 
 *      place - place a block of given size in a block of given pointer.    
 *  	splits given block if the block is large enough for allocated space and new free block  
 *	removes block from the free list. 
 */ 
 static void place(void *bp, size_t asize)
 {
 	void *nxt_bp;
 	//get size of the current free block 
 	size_t csize = GET_SIZE(HDRP(bp));

 	//if the free block is large enough to hold both the allocated block and additional free block
 	if ((csize - asize) >= (2*DSIZE)) {
 		
 		//REMOVE bp form free list
 		remove_free_list(bp);
 		
 		//create header and footer of new allocated block with allocated size.
 		PUT(HDRP(bp), PACK(asize, 1));
 		PUT(FTRP(bp), PACK(asize, 1));
 		
 		//get pointer to the new (smaller) free block
 		nxt_bp = NEXT_BLKP(bp);
 		
 		//Put header and footer of new free block with remaining size.
 		PUT(HDRP(nxt_bp), PACK(csize-asize, 0));
 		PUT(FTRP(nxt_bp), PACK(csize-asize, 0));
 		 		
 		//ADD new free block to free list
 		add_free_list(nxt_bp);
 	}
 	
 	//if block is not large enough to hold allocated block and additional free block
 	else {
 		//replace header and foot of free block with same size as before but allocated. 
 		PUT(HDRP(bp), PACK(csize, 1));
 		PUT(FTRP(bp), PACK(csize, 1));
 		//remove free block from the free list. 
 		remove_free_list(bp);
 	}
 }
 

/* 
 *      remove_free_list - removes a given pointer from the free list.     
 *  	updates free count and global_min_list if necessary  
 */  
 static void remove_free_list(void *bp)
 {	
 	int minlist; 
 	int size;
 	
 	//decrement global free count. 
 	free_count--; 
 	
 	//get size of the block
 	size = GET_SIZE(HDRP(bp));
 	
 	//calculate the minlist for the given block with a max of 83.
 	minlist = size / 50;
 	if(minlist > 83)
 		minlist = 83; 
 	
 	//if the prev free pointer and next free pointer were 0 set global first free pointer to 0.
 	//this indicates that there are no remaining items on this free list. 
	if(GET(bp) == 0 && GET(bp + WSIZE) == 0) { 
		//set this list pointer to 0 indicating no items on the list. 
 		PUT(heap_listp+(minlist * WSIZE), 0);
 		
 		//if this list was the global min list update global minlist.
 		if(global_minlist == minlist) { 
 			// if there are remaining free blocks search for the next list with blocks on its free list
 			if(free_count > 0){
 			int i;
 			//loop through lists until find list that pointer is non zero.
 			for (i = minlist; GET(heap_listp+(i * WSIZE)) == 0; i++);
 			//update global minlist
 			global_minlist = i;
 			}
 			//if there are no remaining free blocks, update global min list to 100.
			else(global_minlist = 100); 			
 		}
 	}
 	
 	//else if the prev pointer was 0 and next not zero make global first free pointer next.
 	//this indicates that this was the first item on the list so the list pointer needs to point to next. 
 	else if (GET(bp) == 0 && GET(bp + WSIZE) != 0){
 		//set list pointer to point to next
 		PUT(heap_listp+(minlist * WSIZE), GET(bp + WSIZE));
 		//set the next free block's previous pointer to be 0 indicating it is first on the list. 
 		PUT((char *)GET(bp + WSIZE), 0);
 	}
 	
 	//else if prev pointer not 0 and next 0 then make prev's next pointer 0.
 	//this indicates that this item was the last item on the list so the previous free block's next pointer must be set to 0.
 	else if (GET(bp) != 0 && GET(bp + WSIZE) == 0) 
 		//set previous free block's next pointer to 0.
 		PUT(((char *)GET(bp) + WSIZE), 0);
 		
 	//else if prev pointer and next pointer not 0 update pointers 
 	//this indicates that this item was in the center of the list somewhere so the previous block and next blocks pointers need to be set to skip it. 
 	else {
 		//set previous free block's next pointer to point to the next free block.
 		PUT(((char *)GET(bp) + WSIZE), GET(bp + WSIZE));	
 		//set next free block's previous pointer to point to previous free block. 
 		PUT(((char *)GET(bp + WSIZE)), GET(bp));	
 	}
 }
 
 
 /* 
 *      add_free_list - adds a given pointer to the free list.       
 *  	updates free count and global_min_list if necessary  
 */  
 static void add_free_list(void *bp)
 {	 
 	int minlist;
 	void *temp_next;
 	void *temp_cur;
 	void *temp_prev;
 	int size;
 	
 	//increment free count
 	free_count++;
 	
 	//get size of the block
 	size = GET_SIZE(HDRP(bp));
 	
 	//calculate the minlist for the given block with a max of 83.
 	minlist = size / 50;
	if(minlist > 83)
		minlist = 83;
	
	//if this min list is less than the current global minlist then this list is the new global free list.  Also if global min list is 100 indicating no free blocks this minlist is now the global free list. (the or is repetitive but kept for clarity of thought process)
	if(global_minlist > minlist || global_minlist == 100)
		global_minlist = minlist; //update global min list
	
	//get the pointer of the first block on the min list free list.  
	temp_cur = (char *)GET(heap_listp + (minlist * WSIZE));
	
	//if the free list is empty 
	if(temp_cur == 0){
		//set free list pointer to point to this block.
		PUT(heap_listp + (minlist * WSIZE), (int)bp);	
		//set this blocks previous free block poitner to 0 indicating it is first on the list.
		PUT(bp, 0); 
		//set this blocks next free block pointer to 0 indicating it is last on the list.  
		PUT(bp+WSIZE, 0);
	}
	
	//else (this list is not free)
	else {
		//initialize temp_prev to point to first item on the list (used in loop) 
		temp_prev = (char *)GET(heap_listp + (minlist * WSIZE));
		
		//find place on list- loop through this free list until temp_cur is greater than or equal to temp_cur.  Keep temp_prev updated with a pointer to the previous block as new block will be placed between temp_prev and temp_cur on free list.   
		for (; (int)temp_cur != 0 && GET_SIZE(HDRP(temp_cur)) < size; temp_cur = (char *)GET(temp_cur+WSIZE))
			temp_prev = temp_cur;
		
		//set temp_cur to temp_prev for simplicities sake.
		temp_cur = temp_prev;
		
		//save the next free block pointer from temp cur because it will be overwritten.
		temp_next = (char *)GET(temp_cur + WSIZE); 
		
		//set the next free block pointer from temp cur to this block
		PUT(temp_cur + WSIZE, (int)bp); 

		//if the saved temp next is not 0 (ie there is a next block) update the next block's previous pointer to point to this block
		if((int)temp_next != 0) 
			PUT(temp_next, (int)bp);
		
		//set the next and previous pointers of this block to the correct values.  
		PUT(bp, (int)temp_cur); 
		PUT(bp+WSIZE, (int)temp_next);
		
		}
}
 

////////////////////////////////////////////////////////////////////


/*
 *     mm_free - Frees block pointed to by given pointer.
 */
void mm_free(void *bp)
{
	//mm_check();
	//calculate size of block to be freed.
	size_t size = GET_SIZE(HDRP(bp));

	//reset header and footer to indicate free allocation status
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	
	//coalesce - combine neighboring free blocks if possible.  Also added to the free list within coalesce.  
	coalesce(bp);
}

 /* 
 *      coalesce - combines newly created free block with neighboring free blocks if possible       
 *  	adds given block to the free list. 
 */  
 static void *coalesce(void *bp)
 {
 	//get allocation status of next and previous free blocks.  
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	
	//get size of this block.  
	size_t size = GET_SIZE(HDRP(bp));

	//if next and previous blocks are allocated 
	if (prev_alloc && next_alloc) { // Case 1 
		
		//add this block to the free list and return.
		add_free_list(bp);
		return bp;
	}

	//else if next block is free but previous block is allocated
	else if (prev_alloc && !next_alloc) {

 		//remove next block from the free list since it will no longer exist
 		remove_free_list(NEXT_BLKP(bp));
 		
 		// add the next blocks size to this blocks size.  
 		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
 		
 		//set header and footer for new combined block with new combined size. 
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));

		//add new combined block to the free list.  
		add_free_list(bp);


	}

	//else if previous block is free but next block is allocated.  
	else if (!prev_alloc && next_alloc) { // Case 3 

 		//remove previous block from the free list since it will no longer exist
 		remove_free_list(PREV_BLKP(bp));

 		// add the previous blocks size to this blocks size. 
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		
		//set header and footer for the new combined block with new combined size. 
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
		
		//set bp to previous block because the header top of the block is now previous  block.
		bp = PREV_BLKP(bp);

		//add new combined block to the free list.  
		add_free_list(bp);

	}
	
	//else if previous block and next block are free. 
	else {  

 		//remove previous block from the free list since it will no longer exist
 		remove_free_list(PREV_BLKP(bp));
 		//remove next block from the free list since it will no longer exist
 		remove_free_list(NEXT_BLKP(bp));

 		//add the previous and next blocks size to this blocks size
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		
		//set header and footer for the new combined block with new combined size. 
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
		
		//set bp to previous block because the header top of the block is now previous  block.
		bp = PREV_BLKP(bp);

		//add new combined block to the free list.  
		add_free_list(bp);

	}
	
	//return pointer to the free block as coalesced 
	return bp;
 }

/*
 *    mm_realloc - resize a given block to a given size and return pointer to the new resize block.  
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    void *temp;
    size_t tempsize;
    size_t copySize;
    
    //get allocation status of next and previous free blocks.  
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(oldptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(oldptr)));
    
    //get size of the current block
    size_t size_prev = GET_SIZE(HDRP(oldptr));
    int increase;
    
    //decide if the new size constitutes and increase or not
    if(size_prev  < size + DSIZE) 
    	    increase = 1;
    else
    	    increase = 0;
    
    //if size is equal to zero, the call is equivalent to mm free(ptr)
    if (size == 0){
	mm_free(ptr);
    	newptr = 0;
    	return NULL;
    }
    
    //if ptr is NULL, the call is equivalent to mm malloc(size)
    if (oldptr == NULL)	
	return mm_malloc(size);

    // if shrinking ptr and released space will be large enough to be a block then shrink allocated block and create new block.  
    if(increase == 0 && (size_prev - size - DSIZE) > (2*DSIZE)){ 
    	    
    	// Adjust block size to include overhead and alignment reqs.
    	if (size <= DSIZE)
    	    	size = 2*DSIZE;
    	 else
	  	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // align size
	
	//if the adjusted size still leaves enough space for a free block continue with creating new free block and shrunk allocated block. 
	if((size_prev - size) > (2*DSIZE)){

	 //reset header and footer of old block for the new (shrunk) size
    	 PUT(HDRP(oldptr), PACK(size, 1)); 
	 PUT(FTRP(oldptr), PACK(size, 1)); 
	 
	 // set new ptr to old ptr
	 newptr = oldptr;
	 
	 // reset old pointer to the new (empty) block
	 oldptr =  (NEXT_BLKP(newptr)); 
	 
	 //set header and footer for new (empty) block with remaining size
	 PUT(HDRP(oldptr), PACK(size_prev - size, 0));
	 PUT(FTRP(oldptr), PACK(size_prev - size, 0));
  	
	//coalesce new (free) block (adds to free list)
	coalesce(oldptr);
	
	//return pointer to shurnk block
	return newptr;
	}
    }
    
    //if shrinking ptr and released space too small to be a block or size is the same return same ptr because no change is necessary 
    if(increase == 0) {
    	    return ptr;
    }
    
    // else if expanding ptr 
    else {
    	    
    	    //if next and prev are unallocated and combining the three blocks would fufill new size requirement merge blocks.
    	    if (next_alloc == 0 && prev_alloc == 0 && ((GET_SIZE(HDRP(PREV_BLKP(oldptr)))) + (GET_SIZE(HDRP(NEXT_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){
    	    	    
    	    	    //set new ptr to the prev block since this will be the address of the expanded block.  
    	   	    newptr = PREV_BLKP(oldptr);
    	   	    
    	   	    //temp set to next block.
    	   	    temp = NEXT_BLKP(oldptr);
    	   	    
    	   	    //temp size is size of next and prev blocks -- ie the added size to the current block.
    	    	    tempsize = GET_SIZE(FTRP(newptr)) + GET_SIZE(FTRP(temp));
    	    	    
    	    	    //remove next block and previous block from the free list since they will no longer exist.  
    	    	    remove_free_list(PREV_BLKP(oldptr));
    	    	    remove_free_list(NEXT_BLKP(oldptr));
    	    	    
    	    	    // Adjust block size to include overhead and alignment reqs.
    	    	    if (size <= DSIZE)
    	    	    	size = 2*DSIZE;
		    else
		    	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); 
		
		    // if not big enough for new free block make new block take all space
    	    	    if((tempsize + size_prev) < (size + 2*DSIZE))
    	    	    	 size = tempsize + size_prev;
    	    	 
	    	    //set header to reflect new (expanded) size.
    	    	    PUT(HDRP(newptr), PACK(size, 1));
    	    	    
    	    	    // calculate copy size and copy memory from old block to new (expanded) block
    	    	    copySize = GET_SIZE(HDRP(oldptr));
    	    	    memcpy(newptr, oldptr, copySize);
    	    	    
    	    	    //set footer to reflect new (expanded) size. 
    	    	    PUT(FTRP(newptr), PACK(size, 1));
    	    	        	    
    	    	    //if new free block initialize i
    	    	    if((tempsize + size_prev) >= (size + 2*DSIZE)){ 
			    
    	    	    	    // set new pointer to the new (empty) block
    	    	    	    temp = NEXT_BLKP(newptr); 
    	    	    	    
    	    	    	    // set header and foot for new (empty) block with remaining size
			    PUT(HDRP(temp), PACK(tempsize + size_prev - size, 0));
			    PUT(FTRP(temp), PACK(tempsize + size_prev - size, 0));

			    //add new (free) block to the free list
			    add_free_list(temp);
    	    	    }
    	    	    //return expanded block.
    	    	    return newptr;    	   	    	    
    	    }    	    
    	    
    	    //if next is unallocated and combining next with this block would fufill new size requirement merge the blocks
    	    else if(next_alloc == 0 && ((GET_SIZE(HDRP(NEXT_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){
    	    	    
    	    	    //temp set to next block
    	    	    temp = NEXT_BLKP(oldptr);
    	    	    
    	    	    //temp size is size of next block
    	    	    tempsize = GET_SIZE(FTRP(temp));
    	    	    
    	    	    //remove next block from the free list since it will no longer exist. 
    	    	    remove_free_list(NEXT_BLKP(ptr));
    	    	    
    	    	    // Adjust block size to include overhead and alignment reqs.
    	    	    if (size <= DSIZE)
    	    	    	size = 2*DSIZE;
		    else
		    	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
		
	            // if not big enough for new free block make new block take all space
    	    	    if((tempsize + size_prev) < (size + 2*DSIZE)) 
    	    	    	 size = tempsize + size_prev;
	    	    
    	    	    //set header and footer for block to reflect new (expanded) sizes
    	    	    PUT(HDRP(oldptr), PACK(size, 1));
    	    	    PUT(FTRP(oldptr), PACK(size, 1)); 
    	    	    
    	    	    //if new free block initialize it
    	    	    if((tempsize + size_prev) >= (size + 2*DSIZE)){ 
    	    	    	    
			    // set new pointer to the new (empty) block
    	    	    	    newptr = NEXT_BLKP(oldptr);
    	    	    	    
    	    	    	    // set header and foot for new (empty) block with remaining size
			    PUT(HDRP(newptr), PACK(tempsize + size_prev - size, 0));
			    PUT(FTRP(newptr), PACK(tempsize + size_prev - size, 0));
			    
			    //add new (free) block to the free list
			    add_free_list(newptr);
    	    	    }
    	    	    //return expanded block.
    	    	    return oldptr;    	    	    
    	    }
    	    
    	    //if prev is unallocated and combining prev with this block would fufill new size requirement merge the blocks
    	    else if(prev_alloc == 0 && ((GET_SIZE(HDRP(PREV_BLKP(oldptr)))) + size_prev) >= (size + DSIZE)){
    	    	    
    	    	    //set new ptr to the prev block since this will be the address of the expanded block.  
    	    	    newptr = PREV_BLKP(oldptr);
    	    	    
    	    	    //tempsize is size of prev block. 
    	    	    tempsize = GET_SIZE(FTRP(newptr));
    	    	    
    	    	    //remove prev block from the free list
    	    	    remove_free_list(PREV_BLKP(oldptr));
    	    	    
    	    	    // Adjust block size to include overhead and alignment reqs.
    	    	    if (size <= DSIZE)
    	    	    	size = 2*DSIZE;
		    else
		    	size =  DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
		
		    // if not big enough for new free block make new block take all space
    	    	    if((tempsize + size_prev) < (size + 2*DSIZE))
    	    	    	 size = tempsize + size_prev;
	    	    
    	    	    //set header to reflect new (expanded) size.
    	    	    PUT(HDRP(newptr), PACK(size, 1)); 
    	    	    
    	    	    // calculate copy size and copy memory from old block to new (expanded) block
    	    	    copySize = GET_SIZE(HDRP(oldptr));
    	    	    memcpy(newptr, oldptr, copySize);
    	    	    
    	    	    //set footer to reflect new (expanded) size.
    	    	    PUT(FTRP(newptr), PACK(size, 1)); // resize old 
    	    	    
    	    	    //if new free block initialize it
    	    	    if((tempsize + size_prev) >= (size + 2*DSIZE)){
    	    	    	    
    	    	    	    // set new pointer to the new (empty) block
			    temp = NEXT_BLKP(newptr);
			    
			    // set header and foot for new (empty) block with remaining size
			    PUT(HDRP(temp), PACK(tempsize + size_prev - size, 0));
			    PUT(FTRP(temp), PACK(tempsize + size_prev - size, 0));
			    
			    //add new (free) block to the free list
			    add_free_list(temp);
    	    	    }
    	    	    //return expanded block.
    	    	    return newptr;    	
    	    }

    	    //else if next and previous blocks are allocated -- default case
    	    
    	    //set new pointer to newly allocated block of size
    	    newptr = mm_malloc(size);
    	    
    	    //calculate copy size with a maximum of size.  (size should never be less than copySize but left in for safety)
    	    copySize = GET_SIZE(HDRP(oldptr));
	    if (size < copySize)
		    copySize = size;
	    
	    //copy memory to new block
	    memcpy(newptr, oldptr, copySize);
	    
	    //free old memory block
	    mm_free(oldptr);
	    
	    //return new memory block.
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
	//size_t* helpR;
	void* tempPtr;
	size_t* topHeap =  mem_heap_lo();	//Get top of heap from mdriver.c
    	size_t* bottomHeap =  mem_heap_hi();	//And get footer of heap
	
	//First, make sure bottomHeap is set right.
	assert(bottomHeap == mem_heap_hi());
//	printf("Top of heap: %p\n Bottom of heap: %p\n", topHeap, bottomHeap);

	// Then, check to ensure that we have no data past mem_heapsize. 
	assert(bottomHeap < (topHeap + mem_heapsize()));
	
	/* Next, make sure we haven't written past mem_heap_hi(). */
//	helpR = bottomHeap + 1;
//	while (helpR < (topHeap + mem_heapsize())) {
//		assert((*helpR) == 0);
//		helpR++;
//	}

  	//While there is a next pointer, go through the heap
	for(tempPtr = topHeap; GET_SIZE(HDRP(tempPtr)) > 0; tempPtr = NEXT_BLKP(tempPtr)) {
	//for(tempPtr = GET_BP(heap_listp); GET_SIZE(HDRP(tempPtr)) > 0; tempPtr = NEXT_BLKP(tempPtr)) {
        	
		if (tempPtr > topHeap){
            		printf("ERROR: Top of heap exceeded by pointer\n top: %p,\n pointer: %p\n", topHeap, tempPtr);
        	}
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
//			printf("Header/footer mismatch:\n");
//			printf("Block %x: Header %d, Footer %d\n", (size_t)tempPtr, GET(HDRP(tempPtr)), GET(FTRP(tempPtr)));
    		}
		if ((size_t)tempPtr%8){
        		printf("Error, %p misaligned our headers and payload\n", tempPtr);
    		}
	}

	// Then, make sure the blocks in our free lists are actually free. 
//	int i;
//	for (i = 0; i < free_count; i++) {
//		tempPtr = (char *)GET(heap_listp + (i * WSIZE));
//		while (tempPtr != NULL) {
//			assert(!GET_ALLOC(tempPtr));
//			//assert(GET_SIZE(tempPtr) < MAX_BLOCK_ALLOCSIZE);
//			tempPtr = (HDRP(NEXT_BLKP(tempPtr)));
//		}
//	}
	return 0;


 }
//////////////////////////////////////////////////////////////////////////////////////////////
