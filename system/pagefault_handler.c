/* pagefault_handler.c - pagefault_handler */

#include <xinu.h>

/* The supplied dispatcher calls with no arguments and discards the error code. */
__attribute__((force_align_arg_pointer))
void pagefault_handler(void)
{
    uint32 address;
    struct procent *prptr;
    uint32 heap_page;

    /* The interrupted code may have left the direction flag set. */
    asm volatile("cld" ::: "cc", "memory");
    address = (uint32)read_cr2();
    prptr = &proctab[currpid];

    /* The mapper also checks that the page is not already present. */
    if (prptr->prvheap_bitmap != NULL && address >= VHEAP_BASE) {
        heap_page = (address - VHEAP_BASE) / PAGE_SIZE;
        if ((prptr->prvheap_bitmap[heap_page / 8] &
             (1U << (heap_page % 8))) != 0 &&
            paging_map_heap_page(address) == OK) {
            return;
        }
    }

    kprintf("P%d:: SEGMENTATION_FAULT\n", currpid);
    kill(currpid);

    panic("Could not terminate faulting process");
}
