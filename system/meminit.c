/* meminit.c - meminit */

#include <xinu.h>

void	*minheap;		/* Start of heap. */
void	*maxheap;		/* Highest valid Xinu memory address. */
/*------------------------------------------------------------------------
 * meminit - initialize memory bounds and the free memory list
 *------------------------------------------------------------------------
 */
void meminit(void)
{
    struct memblk *block;
    /* Start the heap just past the kernel image. */
    uint32 heap_start = (uint32)roundmb(&end);
    uint32 xinu_end = (uint32)maxheap + 1U;
    uint32 heap_end;

    if (xinu_end <= NULLSTK) {
        panic("No space for the Xinu boot stack");
    }

    /* Leave room at the top for the boot stack. */
    heap_end = (uint32)truncmb(xinu_end - NULLSTK);

    /* The heap must fit above the PC memory hole. */
    if (heap_start < (uint32)HOLEEND || heap_start >= heap_end ||
        heap_end - heap_start < sizeof(struct memblk)) {
        panic("Invalid Xinu heap bounds");
    }

    /* Start the free list with one block. */
    minheap = (void *)heap_start;
    block = (struct memblk *)minheap;
    block->mnext = NULL;
    block->mlength = heap_end - heap_start;
    memlist.mnext = block;
    memlist.mlength = block->mlength;
}
