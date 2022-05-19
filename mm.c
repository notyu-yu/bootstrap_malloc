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
    "summer_research",
    /* First member's full name */
    "Yuhang Cui",
    /* First member's email address */
    "yuhang.cui@yale.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define WSIZE 4
#define DSIZE 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define CHUNKSIZE (1<<12) // Heap request chunk
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc)) // Put size and alloc bit in one word

// Read and write to address p
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// Read size and alloc bit at p
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// bp is address of payload
// Get header and footer address of block pointer bp
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Get address of next and previous block of bp
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Start of block list
static void * heap_listp;

// Coalesce free blocks with adjacent free blocks, return pointer to coalesced free block
static void * coalesce(void * bp) {
	// Alloc bit of prev and next block
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	// Current block size
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) {
		// Neither are free
		return bp;
	} else if (prev_alloc && !next_alloc) {
		// Coalesce with next block
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	} else if (!prev_alloc && next_alloc) {
		// Coalesce with previosu block
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0 ));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	} else {
		// Both blocks are free
		size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(HDRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}

// Extend heap by 1 chunk
static void * extend_heap(size_t words) {
	char * bp;
	size_t size;

	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; // Maintain double word alignment
	if ((long)(bp = mem_sbrk(size)) == -1) {
		return NULL;
	}

	// Free block header and footer
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	// New epilogue header
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

	// Coalesce previous block
	return coalesce(bp);
}

// Inline to avoid unused warnings
// First fit search for implicit free list, return pointer to payload section, NULL if no fit found
static inline void *first_fit(size_t asize) {
	void * cur_search = heap_listp;
	// Repeat until epilogue block is reached
	while (!((GET_ALLOC(HDRP(cur_search))==1)&&(GET_SIZE(HDRP(cur_search))==0))) {
		if ((GET_ALLOC(HDRP(cur_search))==0) && (GET_SIZE(HDRP(cur_search)) >= asize)) {
			return cur_search;
		} else {
			cur_search = NEXT_BLKP(cur_search);
		}
	}
	return NULL;
}

// Best fit search for implicit free list, return pointer to payload, NULL if not found
static inline void *best_fit(size_t asize) {
	void * cur_search = heap_listp;
	size_t cur_size = 0;
	void * best_result = NULL;
	size_t best_size = (size_t)-1; // Default to max size
	// Repeat until epilogue block is reached
	while (GET_SIZE(HDRP(cur_search))!=0) {
		cur_size = GET_SIZE(HDRP(cur_search));
		if ((GET_ALLOC(HDRP(cur_search))==0) && (cur_size >= asize)) {
			// Update new best fit pointer and size
			if (GET_SIZE(HDRP(cur_search)) < best_size) {
				best_size = cur_size;
				best_result = cur_search;
			}
		}
		cur_search = NEXT_BLKP(cur_search);
	}
	return best_result;
}

// Place fit algorithm here
static void * find_fit(size_t asize) {
	return best_fit(asize);
}

// Put an asize allocated block at bp
static void place(void * bp, size_t asize) {
	size_t original_size = GET_SIZE(HDRP(bp));
	size_t free_size;
	void * free_p;
	// Check if there is free block leftover 
	if (GET_SIZE(HDRP(bp)) > (asize + DSIZE)) {
		// Split block into allocated and free blocks
		free_size = original_size-asize;
		// Header and footer of free block
		free_p = (char *)(bp) + asize;
		PUT(HDRP(free_p), PACK(free_size, 0));
		PUT(FTRP(bp), PACK(free_size, 0));
		// Header and footer of allocated block
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
	} else {
		// Allocate entire block
		PUT(HDRP(bp), PACK(original_size, 1));
		PUT(FTRP(bp), PACK(original_size, 1));
	}
}

// Shrink current block
static void shrink_blk(void * bp, size_t asize) {
	size_t original_size = GET_SIZE(HDRP(bp));
	size_t free_size;
	void * free_p;
	// Check if there is free block leftover 
	if (original_size > (asize + DSIZE)) {
		// Split block into allocated and free blocks
		free_size = original_size-asize;
		// Header and footer of free block
		free_p = (char *)(bp) + asize;
		PUT(HDRP(free_p), PACK(free_size, 0));
		PUT(FTRP(bp), PACK(free_size, 0));
		// Header and footer of allocated block
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		coalesce(free_p);
	}
	// No change needed otherwise
}

int mm_init(void)
{
	// Allocate initial heap
	if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) {
		return -1;
	}
	PUT(heap_listp, 0); //Padding
	// Prologue header and footer
	PUT(heap_listp + WSIZE, PACK(DSIZE, 1));
	PUT(heap_listp + WSIZE*2, PACK(DSIZE, 1));
	// Epilogue header
	PUT(heap_listp + WSIZE*3, PACK(0, 1));
	heap_listp += 2*WSIZE;

	// Extend heap by a chunk
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL){
		return -1;
	}

    return 0;
}

void *mm_malloc(size_t size)
{
	/*
	// Default implementation
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
	*/
	
	size_t asize; // Adjusted block size
	size_t extendsize; // Amount to extend heap by
	char * bp;

	// Ignore 0 size
	if (size == 0) {
		return NULL;
	}

	// Add overhead and alignment to block size
	if (size <= DSIZE) {
		asize = 2*DSIZE;
	} else {
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE); // Add overhead and make rounding floor
	}

	// Search free block for fit
	if ((bp = find_fit(asize)) != NULL) {
		assert(GET_ALLOC(HDRP(bp))==0);
		place(bp, asize);
		return bp;
	}

	// Need to extend heap
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
		// Not enough memory
		return NULL;
	}
	assert(GET_ALLOC(HDRP(bp))==0);
	place(bp, asize);
	return bp;
}

void mm_free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));
	
	// Set header and footer to free
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	coalesce(ptr);
}

void *mm_realloc(void *ptr, size_t size)
{
	/*
	// Default implementation
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
	*/
    void *oldptr = ptr;
    void *newptr;
	size_t blk_size;
	size_t asize;

	// Special cases
	if (ptr == NULL) {
		newptr = mm_malloc(size);
		return newptr;
	}
	if (size == 0) {
		mm_free(ptr);
		return ptr;
	}

	blk_size = GET_SIZE(HDRP(oldptr));

	// Add overhead and alignment to block size
	if (size <= DSIZE) {
		asize = 2*DSIZE;
	} else {
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE); // Add overhead and make rounding floor
	}

	if (blk_size < asize) {
		// Malloc new block
		newptr = mm_malloc(size);
		if (newptr == NULL)
		  return NULL;
		memcpy(newptr, oldptr, size);
		mm_free(oldptr);
		return newptr;
	} else if (blk_size > asize) {
		// Need to shrink block
		shrink_blk(oldptr, asize);
		return oldptr;
	} else {
		// Do nothing
		return ptr;
	}
}
