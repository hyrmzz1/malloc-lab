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
 * mm_init - initialize the malloc package. 최초 가용 블록으로 힙 생성하기.
 */
int mm_init(void)
{
    // Create the initial empty heap
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) // mem_sbrk(size) 함수 호출하면 운영체제에게 size 만큼의 추가 메모리 할당을 요청함. 성공시 가용 메모리 공간 size만큼 늘리고 그 확장된 메모리 공간의 시작 주소를 반환.
        return -1;
    // 블록 구조(순서): Prologue Header - Prologue Footer - Payload - Epilogue Header
    PUT(heap_listp, 0); // 데이터 정렬 위한 공간 만들기 위해 첫 번째 주소에 0 넣음
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    // Prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    // Prologue footer. 헤더와 동일한 val 넣기. (일관성 검사 위해 헤더와 풋터에 동일한 값 할당)
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));    // Epilogue header. 힙의 끝 표시. size 0 => 다음 블록 없음을 의미.
    heap_listp += (2*WSIZE);    // 프롤로그 건너 뛰고 실제 데이터 할당 시작되는 위치로 heap_listp 이동

    /*  Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)   // 초기힙 CHUNKSIZE만큼 확장. 힙 확장 실패시 NULL 반환
        return -1;  // 초기화 실패 => -1 반환
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

// 새 가용 블록으로 힙 확장하기.
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;   // 8(= 더블 워드)의 배수인 블록 리턴하므로
    if ((long)(bp = mem_sbrk(size)) == -1)  // 힙 확장 성공시 확장된 메모리 블록의 시작 주소 반환(=> 변수 bp에 할당됨), 실패하면 -1 반환.
        return NULL;
    
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));   // free block header
    PUT(FTRP(bp), PACK(size, 0));   // free block footer. 헤더와 동일한 size로 초기화!
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   // new epilogue header. 다음 블록의 헤더 주소에 에필로그 헤더 설정. <= why ???

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * mm_free - Freeing a block does nothing. 블록 반환.
 */
void mm_free(void *ptr) // ptr == bp
{
    size_t size = GET_SIZE(HDRP(ptr));

    // 블록의 header & footer free로 변경 => free 두 번째 parameter 0으로
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), pack(size, 0));
    coalesce(ptr);
}

// 경계 태그(footer)로 연결 (블록 통합)
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 이전 블록 풋터의 status => 이전 블록의 할당 상태
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 다음 블록 할당 상태
    size_t size = GET_SIZE(HDRP(bp));   // 현재 블록 사이즈

    // case 1 - 전후 블록 둘다 할당
    if (prev_alloc && next_alloc) {
        return bp;  // 연결 X. 현재 블록의 bp만 반환
    }

    // case 2 - 이전 블록 할당, 다음 블록 free
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));  // 현재 블록 + 다음 블록
        PUT(HDRP(bp), PACK(size, 0));   // 현재 블록의 헤더에 새로운 크기 정보(size) 설정. bp => 새로운 블록(현재 블록 + 다음 블록) 나타냄.
        PUT(FTRP(bp), PACK(size,0));    // 병합한 블록의 풋터 만들어 블록 끝부분에 추가 => 왜 FTRP(NEXT_BLKP(bp))가 아니지?
    }

    // case 3 - 이전 블록 free, 다음 블록 할당
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));  // 이전 블록 + 현재 블록
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // case 4 - 전후 블록 둘다 free
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
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