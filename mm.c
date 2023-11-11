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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
// #define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 가용 리스트 조작을 위한 기본 상수 및 매크로 정의
/* #define 지시문은 매크로명과 매크로 정의(또는 치환될 내용)로 이루어짐.
매크로 내에서 수행된 일련의 연산이나 동작(=> 매크로 정의 부분에 들어가는)은 해당 위치에 *대체*되고, 반환값을 갖는 것이 아닌 단순히 코드의 일부로 *치환*됨.
함수와는 다르게 반환값을 갖지 않고, 해당 위치에 매크로가 정의한 동작이나 연산이 대신 수행됨.
다시 말해 매크로는 매크로 정의 부분에 들어가는 일련의 연산이나 동작 수행 후 코드에서 해당 매크로가 사용된 곳에 매크로 정의 내용이 그대로 대체되므로 
매크로가 호출된 위치에 매크로 정의의 내용이 복사되는 것을 의미함. */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) (1<<12 == 4096 == 4KB) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit(= 0(가용) 또는 1(할당)) into a word */
#define PACK(size, alloc) ((size) | (alloc))    // '|'는 비트 OR 연산자. size와 alloc을 비트 단위로 OR 연산해 하나의 워드로 만듦.

/* Read and write a word at address p. 메모리 블록의 값 읽고 쓸 때 사용. */
#define GET(p) (*(unsigned int *)(p))   // p가 참조하는 워드 읽어서 리턴
#define PUT(p, val) (*(unsigned int *)(p) = (val))  // p가 가리키는 워드에(= 주소 p에) val 저장

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) // 주소 p에 있는 헤더 or 풋터의 size 리턴
#define GET_ALLOC(p) (GET(p) & 0x1) // 주소 p에 있는 헤더 or 풋터의 할당 비트 리턴

/* Given block ptr (bp), compute address of its header and footer. 블록 헤더와 풋터 가리키는 포인터 리턴. => 헤더, 풋터 위치 계산하기 위해 사용됨 */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr (bp), compute address of next and previous blocks (bp 각각 반환) */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

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
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}