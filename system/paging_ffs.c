/* paging_ffs.c - private heap mappings and FFS frame accounting */

#include <xinu.h>

/* NULLPROC never owns a private heap frame and marks free entries. */
static pid32 frame_owner[MAX_FFS_SIZE];
static uint32 free_frames = (MAX_FFS_SIZE);
static uint32 next_frame;

/* Find a free frame. Resume each search after the previous allocation. */
static uint32 alloc_ffs_frame(pid32 pid)
{
    uint32 index;

    if (free_frames == 0) {
        return 0;
    }

    while (frame_owner[next_frame] != NULLPROC) {
        next_frame = (next_frame + 1) % (MAX_FFS_SIZE);
    }
    index = next_frame;
    frame_owner[index] = pid;
    next_frame = (index + 1) % (MAX_FFS_SIZE);
    free_frames--;
    return FFS_FIRST_FRAME + index;
}

status paging_map_heap_page(uint32 address)
{
    struct procent *prptr = &proctab[currpid];
    uint32 page = address >> PAGE_SHIFT;
    uint32 previous_pdbr;
    pd_t *directory;
    pd_t *pde;
    pt_t *table;
    pt_t *pte;
    uint32 frame;

    if (prptr->prvheap_bitmap == NULL || address < VHEAP_BASE) {
        return SYSERR;
    }

    previous_pdbr = paging_enter_system();
    if (previous_pdbr != prptr->prpdbr) {
        paging_leave_system(previous_pdbr);
        return SYSERR;
    }
    directory = (pd_t *)prptr->prpdbr;
    pde = &directory[page / PT_ENTRIES];

    /* vmalloc supplies the second-level table when it reserves the page. */
    if (!pde->pd_pres) {
        paging_leave_system(previous_pdbr);
        return SYSERR;
    }

    table = (pt_t *)((uint32)pde->pd_base << PAGE_SHIFT);
    pte = &table[page % PT_ENTRIES];
    if (pte->pt_pres) {
        paging_leave_system(previous_pdbr);
        return SYSERR;
    }
    frame = alloc_ffs_frame(currpid);
    if (frame == 0) {
        paging_leave_system(previous_pdbr);
        return SYSERR;
    }

    pte->pt_base = frame;
    pte->pt_write = 1;
    pte->pt_pres = 1;
    paging_leave_system(previous_pdbr);

    /* Zero the frame through the new mapping before resuming the child. */
    memset((void *)(page << PAGE_SHIFT), 0, PAGE_SIZE);
    return OK;
}

status paging_release_heap_pages(pid32 pid, uint32 first_page, uint32 npages)
{
    intmask mask = disable();
    uint32 page, end, previous_pdbr;
    pd_t *directory;
    uint32 table_end;
    uint32 frame;
    uint32 index;
    pd_t *pde;
    pt_t *table;
    pt_t *pte;

    if (isbadpid(pid) || proctab[pid].prvheap_bitmap == NULL ||
        first_page > VHEAP_PAGE_COUNT ||
        npages > VHEAP_PAGE_COUNT - first_page) {
        restore(mask);
        return SYSERR;
    }

    page = (XINU_PAGES) + first_page;
    end = page + npages;
    previous_pdbr = paging_enter_system();
    directory = (pd_t *)proctab[pid].prpdbr;

    while (page < end) {
        pde = &directory[page / PT_ENTRIES];
        table_end = (page / PT_ENTRIES + 1) * PT_ENTRIES;
        if (table_end > end) {
            table_end = end;
        }
        if (!pde->pd_pres) {
            page = table_end;
            continue;
        }

        table = (pt_t *)((uint32)pde->pd_base << PAGE_SHIFT);
        for (; page < table_end; page++) {
            pte = &table[page % PT_ENTRIES];
            if (!pte->pt_pres) {
                continue;
            }

            frame = pte->pt_base;
            index = frame - FFS_FIRST_FRAME;
            if (index >= (MAX_FFS_SIZE) || frame_owner[index] != pid) {
                panic("Invalid private heap frame");
            }
            memset(pte, 0, sizeof(*pte));
            frame_owner[index] = NULLPROC;
            free_frames++;
        }
    }
    /* Restoring CR3 flushes the current private heap translations. */
    paging_leave_system(previous_pdbr);
    restore(mask);
    return OK;
}

uint32 free_ffs_pages(void)
{
    intmask mask = disable();
    uint32 count = free_frames;

    restore(mask);
    return count;
}

uint32 used_ffs_frames(pid32 pid)
{
    intmask mask = disable();
    uint32 count = 0;
    uint32 index;

    if (pid != NULLPROC && !isbadpid(pid)) {
        for (index = 0; index < (MAX_FFS_SIZE); index++) {
            if (frame_owner[index] == pid) {
                count++;
            }
        }
    }
    restore(mask);
    return count;
}

uint32 allocated_virtual_pages(pid32 pid)
{
    intmask mask = disable();
    uint32 count = (XINU_PAGES);
    uint32 index;
    byte *bitmap;
    byte bits;

    if (isbadpid(pid)) {
        restore(mask);
        return 0;
    }

    /* ECE565 counts the shared Xinu pages plus private reservations. */
    bitmap = proctab[pid].prvheap_bitmap;
    if (bitmap != NULL) {
        for (index = 0; index < VHEAP_BITMAP_SIZE; index++) {
            bits = bitmap[index];
            while (bits != 0) {
                bits &= bits - 1U;
                count++;
            }
        }
    }

    restore(mask);
    return count;
}
