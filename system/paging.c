/* paging.c - paging_build_system */

#include <xinu.h>

#define PT_ENTRIES 1024U
#define PAGE_SHIFT 12U
#define PD_SHIFT   22U
#define ADDR_MAX   0xFFFFFFFFU

static uint32 next_pt_frame;
static uint32 pt_pool_end;

/* Take a frame from the pool and clear its entries. */
static void *alloc_boot_pt_frame(void)
{
    void *frame;

    if (next_pt_frame >= pt_pool_end) {
        return NULL;
    }

    frame = (void *)next_pt_frame;
    next_pt_frame += PAGE_SIZE;
    memset(frame, 0, PAGE_SIZE);
    return frame;
}

/* Map a 4 KiB virtual page to its own physical address. */
static status identity_map_page(pd_t *directory, uint32 address)
{
    uint32 directory_index = address >> PD_SHIFT;
    uint32 table_index = (address >> PAGE_SHIFT) & (PT_ENTRIES - 1U);
    pd_t *pde = &directory[directory_index];
    pt_t *table;

    /* Each page table covers 4 MiB. */
    if (!pde->pd_pres) {
        table = alloc_boot_pt_frame();
        if (table == NULL) {
            return SYSERR;
        }

        pde->pd_base = (uint32)table >> PAGE_SHIFT;
        pde->pd_write = 1;
        pde->pd_pres = 1;
    } else {
        table = (pt_t *)((uint32)pde->pd_base << PAGE_SHIFT);
    }

    /* Base fields store physical frame numbers. */
    table[table_index].pt_base = address >> PAGE_SHIFT;
    table[table_index].pt_write = 1;
    table[table_index].pt_pres = 1;
    return OK;
}

#ifdef PAGING_BOOT_DEBUG
/* Summarize each populated table and check every present translation. */
static void dump_system_directory(pd_t *directory)
{
    uint32 i, j, pages, identity_pages, expected_frame;
    pt_t *table;

    kprintf("System page directory at %08X\n", (uint32)directory);
    for (i = 0; i < PT_ENTRIES; i++) {
        if (!directory[i].pd_pres) {
            continue;
        }

        table = (pt_t *)((uint32)directory[i].pd_base << PAGE_SHIFT);
        pages = identity_pages = 0;
        for (j = 0; j < PT_ENTRIES; j++) {
            if (table[j].pt_pres) {
                pages++;
                expected_frame = i * PT_ENTRIES + j;
                if (table[j].pt_base == expected_frame) {
                    identity_pages++;
                }
            }
        }
        kprintf("  PDE %u VA %08X table %08X: %u present, %u identity\n",
                i, i << PD_SHIFT, (uint32)table, pages, identity_pages);
    }
}
#endif

pd_t *paging_build_system(void)
{
    uint32 xinu_end;
    uint32 address;
    pd_t *directory;

    if (sizeof(pd_t) * PT_ENTRIES != PAGE_SIZE ||
        sizeof(pt_t) * PT_ENTRIES != PAGE_SIZE ||
        (XINU_PAGES) > ADDR_MAX / PAGE_SIZE) {
        return NULL;
    }

    xinu_end = (XINU_PAGES) * PAGE_SIZE;
    if ((MAX_PT_SIZE) > (ADDR_MAX - xinu_end) / PAGE_SIZE) {
        return NULL;
    }

    /* Put the table pool right after the shared Xinu memory. */
    next_pt_frame = xinu_end;
    pt_pool_end = xinu_end + (MAX_PT_SIZE) * PAGE_SIZE;

    directory = alloc_boot_pt_frame();
    if (directory == NULL) {
        return NULL;
    }

    /* Identity-map Xinu memory and the table pool. */
    for (address = 0; address < pt_pool_end; address += PAGE_SIZE) {
        if (identity_map_page(directory, address) == SYSERR) {
            return NULL;
        }
    }

#ifdef PAGING_BOOT_DEBUG
    dump_system_directory(directory);
#endif
    return directory;
}
