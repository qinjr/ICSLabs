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
    "515030910475",
    /* First member's full name */
    "QinJiarui",
    /* First member's email address */
    "qinjr@icloud.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* ----------------------------------------------------DESCRIPTION---------------------------------------------------- */

/* I use explicit segregated free lists with 5 sizes classes. */
/* My free block structure: 4 bits header + payload + padding + 4 bits prev_blkp + 4 bits next_blkp + 4 bits footer */
/* My allocated block structure: 4 bits header + payload + padding + 4 bits footer */
/* I use last-in-first-out to manage the segregated free lists */
/* I use 2 global variables */
/* I got 91/100 in Performance index */


/* ----------------------------------------------------MACRO DEFINITION---------------------------------------------------- */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) % ALIGNMENT == 0) ? (size) : (((size) / ALIGNMENT + 1) * ALIGNMENT))


/* word and double size(bytes) */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE   4112

#define MAX(x, y)   ((x) > (y) ? (x) : (y))

/* pack size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))

/* read and write a word at address p */
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

/* read size and allocted field from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* given block ptr bp, compute address of its header, footer, pred and succ */
#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define PREDP(bp)   ((char *)FTRP(bp) - DSIZE)
#define SUCCP(bp)   ((char *)FTRP(bp) - WSIZE)

/* given block ptr bp, compute address of next and previous blocks' addresses */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* get list head according to size_class */
#define LIST_HEAD(size_class)   (startp + size_class * 2 * DSIZE)

/* given block ptr bp, check whether it is a legal block ptr which can be freed */
#define ISLEGAL(bp) (((unsigned int)(bp) >= ((unsigned int)heap_listp)) && ((unsigned int)(bp) <= ((unsigned int)heap_listp + mem_heapsize())) && ((unsigned int)(bp) % 8 == 0) && (GET_ALLOC(HDRP(bp)) == 1))


/* ----------------------------------------------------UTILITIES---------------------------------------------------- */

static char *heap_listp;
static char *startp;

/*
 * get_size_class - decide which free list the block should be added to according to its block size
 */
static int get_size_class(unsigned int size) {
    if (size <= 512)
        return 0;
    else if (size >= 513 && size <= 1024)
        return 1;
    else if (size >= 1025 && size <= 2048)
        return 2;
    else if (size >= 2049 && size <= 4112)
        return 3;
    else if (size >= 4113 && size <= 8192)
        return 4;
    else if (size >= 8193 && size <= 16384)
        return 5;
    else
        return 6;
}

/*
 * add_block - add a block to a specific free list. Insert the new block to the list header. Last-in-first-out method
 */
static void add_block(char *bp) {
    if (bp == NULL)
        return;

    int size_class = get_size_class(GET_SIZE(HDRP(bp)));

    if (GET(SUCCP(LIST_HEAD(size_class))) != (int)NULL) {
        PUT(PREDP(GET(SUCCP(LIST_HEAD(size_class)))), bp);
        PUT(SUCCP(bp), GET(SUCCP(LIST_HEAD(size_class))));

        PUT(SUCCP(LIST_HEAD(size_class)), bp);
        PUT(PREDP(bp), LIST_HEAD(size_class));

        return;
    }

    else {
        PUT(SUCCP(LIST_HEAD(size_class)), bp);
        PUT(PREDP(bp), LIST_HEAD(size_class));
        PUT(SUCCP(bp), 0);
        return;
    }
}

/*
 * delete_block - delete a block from original free list
 */
static void delete_block(char *bp) {
    if (bp != NULL) {
        if (GET(SUCCP(bp)) != (int)NULL) {
            PUT(SUCCP(GET(PREDP(bp))), GET(SUCCP(bp)));
            PUT(PREDP(GET(SUCCP(bp))), GET(PREDP(bp)));
            return;
        }
        else {
            PUT(SUCCP(GET(PREDP(bp))), 0);
            return;
        }
    }
    else
        printf("NULL ptr deleted!!\n");
}



/*
 * coalesce - coalesce adjacent free blocks, delete or add blocks to a free list
 */
static void *coalesce(char *bp) {
    if (bp == NULL)
        return NULL;

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    char *prev_blkp = NULL;
    char *next_blkp = NULL;

    if (prev_alloc && next_alloc){}

    else if (prev_alloc && !next_alloc) {
        next_blkp = NEXT_BLKP(bp);
        size += GET_SIZE(HDRP(next_blkp));

        delete_block(next_blkp);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(next_blkp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {
        prev_blkp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(prev_blkp));

        delete_block(prev_blkp);

        PUT(HDRP(prev_blkp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        bp = prev_blkp;
    }

    else {
        prev_blkp = PREV_BLKP(bp);
        next_blkp = NEXT_BLKP(bp);

        size += GET_SIZE(HDRP(prev_blkp)) + GET_SIZE(HDRP(next_blkp));

        delete_block(prev_blkp);
        delete_block(next_blkp);

        PUT(HDRP(prev_blkp), PACK(size, 0));
        PUT(FTRP(next_blkp), PACK(size, 0));

        bp = prev_blkp;
    }

    add_block(bp);
    return bp;
}

/*
 * extend_heap - extend an empty heap, coalesce the adjacent blocks which are free, add it to free list
 */

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    return coalesce(bp);
}

/*
 * find_fit - find the fit block in free list
 * we use best fit to find fit block in the free
 * lists
 */
static char *find_fit(unsigned int block_size) {
    int size_class = get_size_class(block_size);
    int i;
    int diff = 1000000;
    char *best_fit = NULL;
    for (i = size_class; i <= 6; ++i) {
        char *bp = (char *)GET(SUCCP(LIST_HEAD(i)));
        while (bp != NULL) {
            if ((int)GET_SIZE(HDRP(bp)) - (int)block_size < diff && GET_SIZE(HDRP(bp)) >= block_size){
                best_fit = bp;
                diff = GET_SIZE(HDRP(bp)) - block_size;
            }
            else
                bp = (char *)GET(SUCCP(bp));
        }
    }

    return best_fit;
}

/*
 * place - place and divide the block, return the ptr to new free block or return NULL if we can't divide it
 */
static char* place(char *bp, unsigned int block_size) {
    unsigned int oldsize = GET_SIZE(HDRP(bp));
    delete_block(bp);

    if (oldsize - block_size <= 16) {
        PUT(HDRP(bp), PACK(oldsize, 1));
        PUT(FTRP(bp), PACK(oldsize, 1));
        return NULL;
    }
    else {
        PUT(HDRP(bp), PACK(block_size, 1));
        PUT(FTRP(bp), PACK(block_size, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(oldsize - block_size, 0));
        PUT(FTRP(bp), PACK(oldsize - block_size, 0));
        return bp;
    }
}

int mm_check();

/* ----------------------------------------------------MAIN PART---------------------------------------------------- */
/* 
 * mm_init - initialize the malloc package.
 * Structure of the initial heap: padding(4 bits) + 5 blocks which are all list head(16 bits each) + endFlag(4 bits)
 */
int mm_init(void) {
    if ((heap_listp = mem_sbrk(15 * DSIZE)) == (void*)-1)
        return -1;

    startp = heap_listp + DSIZE;
    PUT(heap_listp, 0); //padding
    int i;
    for (i = 0; i < 7; ++i) {
        PUT(heap_listp + 1 * WSIZE, PACK(2 * DSIZE, 1));    //16/1
        PUT(heap_listp + 2 * WSIZE, 0); //pred
        PUT(heap_listp + 3 * WSIZE, 0); //succ
        PUT(heap_listp + 4 * WSIZE, PACK(2 * DSIZE, 1));    //16/1
        heap_listp += 4 * WSIZE;
    }

    heap_listp += WSIZE;
    PUT(heap_listp, PACK(0, 1));//end

    char* bp_init = (char*)extend_heap(CHUNKSIZE / WSIZE);

    if (bp_init == NULL)
        return -1;
    
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a m.ultiple of the alignment.
 *     First, I check whether I can find a free block from a free list,
 *     if I can, return the block ptr.Else, extend a even larger heap and
 *     return the block ptr.
 */
void *mm_malloc(size_t size) {
    //mm_check();
    if (size <= 0)
        return NULL;
    char *bp;
    unsigned int block_size = 2 * DSIZE + ALIGN(size);

    if ((bp = find_fit(block_size)) != NULL) {
        coalesce(place(bp, block_size));
        return bp;
    }

    /* no fit found */
    unsigned int extendsize = MAX(block_size, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    
    coalesce(place(bp, block_size));
    return bp;
}

/*
 * mm_free - Free a block
 *     First, change the block content to make it a free block, then coalesce it
 *     and add it to a free list
 */
void mm_free(void *bp) {
    //mm_check();
    if (ISLEGAL(bp)) {
        size_t size = GET_SIZE(HDRP(bp));

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        coalesce(bp);
    }
    else
        printf("error in free, not a legal block ptr!!!\n");
    
}

/*
 * mm_realloc - The behaviour of realloc function depends on ptr, size and 
 *     the adjacent blocks of ptr.
 *     if ptr == NULL, mm_malloc(size)
 *     if size == 0, mm_free(ptr)
 *     if needed size <= old size, we just simply return the oldptr
 *     if next block of ptr is free and the sum of sizes bigger than needed size, coalesce 2 blocks and space left put in free list
 *     if previous block of ptr is free and the sum of sizes bigger, coalesce and add the left space to free list
 *     if we can't coalesce, we have to malloc a new block and free the oldptr
 */
void *mm_realloc(void *ptr, size_t size) {
    //mm_check();
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    char *oldptr = ptr;
    char *newptr;
    int need_size = ALIGN(size) + 8;
    int old_size = GET_SIZE(HDRP(oldptr));
    if (need_size <= old_size)
        return oldptr;

    char *next_blkp = NEXT_BLKP(oldptr);
    int next_size = GET_SIZE(HDRP(next_blkp));

    if (next_size + old_size >= need_size && GET_ALLOC(HDRP(next_blkp)) == 0) {
        delete_block(next_blkp);
        PUT(HDRP(oldptr), PACK(need_size, 1));
        PUT(FTRP(oldptr), PACK(need_size, 1));

        PUT(HDRP(NEXT_BLKP(oldptr)), PACK(next_size + old_size - need_size, 0));
        PUT(FTRP(NEXT_BLKP(oldptr)), PACK(next_size + old_size - need_size, 0));
        add_block(NEXT_BLKP(oldptr));

        newptr = oldptr;
        return newptr;
    }

    char *prev_blkp = PREV_BLKP(oldptr);
    int pre_size = GET_SIZE(HDRP(prev_blkp));

    if (pre_size + old_size >= need_size + 16 && GET_ALLOC(HDRP(prev_blkp)) == 0) {
        delete_block(prev_blkp);
        PUT(HDRP(prev_blkp), PACK(pre_size + old_size, 1));
        memcpy(prev_blkp, oldptr, old_size - 8);
        PUT(FTRP(prev_blkp), PACK(pre_size + old_size, 1));
        
        newptr = prev_blkp;
        return newptr;
    }

    else {
        size_t copySize;
        newptr = mm_malloc(size);
        if (newptr == NULL)
            return NULL;
        copySize = (size_t)(GET_SIZE(HDRP(oldptr)) - DSIZE);
        if (copySize > size)
            copySize = size;
        memcpy(newptr, oldptr, copySize);
        mm_free(oldptr);
        return newptr;
    }
}



/* ----------------------------------------------------CHECK---------------------------------------------------- */

/*
 * check_block - check a single block pointed by bp for:
 * 1, whether its header match its footer                         
 * 2, whether it is aligned to 8 bytes                            
 * 3, whether the ptr in a free block pointing to a valid address
 */ 
static int check_block(char *bp) {
    if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp)) || GET_ALLOC(HDRP(bp)) != GET_ALLOC(FTRP(bp))) {
        printf("block header not match block footer\n");
        return 0;
    }
    if ((int)bp % DSIZE != 0) {
        printf("not aligned to double word size\n");
        return 0;
    }
    if (GET_ALLOC(HDRP(bp)) == 0) {
        if ((int)SUCCP(bp) <= (int)mem_heap_lo() || (int)SUCCP(bp) >= (int)mem_heap_hi() ||
         (int)PREV_BLKP(bp) <= (int)mem_heap_lo() || (int)PREV_BLKP(bp) >= (int)mem_heap_hi()) {
            printf("this block:%p has illegal ptr to another free block\n", bp);
            return 0;
        }
    }
    return 1;
}

/*
 * check_heap - check:
 * 1, whether every block is good
 * 2, whether every block in the free list marked as free
 */
static int check_heap(void) {
    //check every block if it is correct
    char *bp = heap_listp + WSIZE;
    while (GET_SIZE(HDRP(bp)) > 0) {
        if (check_block(bp) == 1)
            bp = NEXT_BLKP(bp);
        else
            return 0;
    }
    //check if every block in the free list marked as free
    int i;
    for (i = 0; i < 5; ++ i) {
        bp = (char *)GET(SUCCP(startp + i * 2 * DSIZE));
        while (bp != NULL) {
            if (GET_ALLOC(HDRP(bp)) != 0) {
                printf("a free block at %p is actually not free\n", bp);
                return 0;
            }
            bp = (char *)GET(SUCCP(bp));
        }
    }
    return 1;
}

/*
 * mm_check - check the heap by calling check_heap()
 */
int mm_check(void) {
    if (check_heap()) {
        printf("check completed successfully\n");
        return 1;//check good
    }
    return 0;//check bad   
}

