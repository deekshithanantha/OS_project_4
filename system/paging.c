/* paging.c - paging_build_system */

#include <xinu.h>

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
    if ((MAX_FFS_SIZE) > VIRTUAL_PAGE_COUNT - pt_pool_end / PAGE_SIZE) {
        return NULL;
    }

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

// Track useable frames in the Page Table
// 0-> free, 1 ->use
static byte pt_frame_used[MAX_PT_SIZE];



/* Keep interrupts disabled until paging_leave_system() restores CR3. */
uint32 paging_enter_system(void) {

    uint32 previous_pdbr = (uint32) read_cr3();

    if(previous_pdbr != system_pdbr) {
        write_cr3(system_pdbr);
    }

    return previous_pdbr;

}

/* Restore the directory that was active before borrowing the system map. */

void paging_leave_system(uint32 previous_pdbr) {

    if(previous_pdbr != system_pdbr) {
        write_cr3(previous_pdbr);
    }
}



// To allocate 4kb of frame size for a pagetable or a directory
// Return Physical addrss, or NULL if the pool is full


uint32 paging_alloc_pt_frame(void) {

    uint32 frame;
    uint32 index = 0;

    for(frame = next_pt_frame; frame < pt_pool_end; frame += PAGE_SIZE, index++) {

        if(!pt_frame_used[index])
        {
            pt_frame_used[index] = 1;
            memset((void *)frame, 0, PAGE_SIZE);
            return frame;
        }
    }
    return 0;
}

// A function to free a frame, must clear off any active refrences to it
// before freeing it

status paging_free_pt_frame(uint32 frame) {
    uint32 index ;

    // To avoid addresses outside the reusable pool and
    // addresses in the middle of the frame.

    if(frame < next_pt_frame || frame >= pt_pool_end || (frame & (PAGE_SIZE - 1U)) != 0)
    {
        return SYSERR;
    }


    index = (frame - next_pt_frame) / PAGE_SIZE;

    // to make sure page is free
    if(!pt_frame_used[index]) {
        return SYSERR;
    }

    // The allocator will zero this frame when someone uses it again
    pt_frame_used[index] = 0;
    return OK;
}

// a Function to build and address space when vcreate is called
// return physcical address, or NUll on failures

uint32 paging_create_user_directory(void) {

    intmask mask = disable();
    uint32 previous_pdbr = paging_enter_system();
    uint32 pdbr = paging_alloc_pt_frame();


    uint32 shared_tables = (XINU_PAGES) / PT_ENTRIES;
    uint32 shared_tail = (XINU_PAGES) % PT_ENTRIES;



    pd_t *system_directory = (pd_t *) system_pdbr;
    pd_t *directory = (pd_t *) pdbr;

    uint32 table_frame;
    pt_t *system_table;

    if(pdbr !=0) {
        memcpy(directory, system_directory, shared_tables * sizeof(pd_t));

        if (shared_tail != 0) {
            table_frame = paging_alloc_pt_frame();

            if (table_frame == 0)
            {
                paging_free_pt_frame(pdbr);
                pdbr = 0;
            }
            else {

                system_table = (pt_t *) ((uint32)system_directory[shared_tables].pd_base  << PAGE_SHIFT);

                memcpy((void *)table_frame, system_table, shared_tail * sizeof(pt_t));

                directory[shared_tables] = system_directory[shared_tables];
                directory[shared_tables].pd_base = table_frame >> PAGE_SHIFT;
            }
        }

    }

    paging_leave_system(previous_pdbr);
    restore(mask);
    return pdbr;
}

// Release a private direcory and its owned 2nd level tables


status paging_destroy_user_directory(uint32 pdbr)
{
    intmask mask = disable();
    uint32 previous_pdbr = (uint32)read_cr3();


    uint32 first_owned = (XINU_PAGES) / PT_ENTRIES;
    uint32 index;
    uint32 table_frame;
    pd_t *directory;


    if (pdbr == previous_pdbr || pdbr < next_pt_frame || pdbr >= pt_pool_end || (pdbr & (PAGE_SIZE - 1U)) != 0 ||
        !pt_frame_used[(pdbr - next_pt_frame) / PAGE_SIZE])
    {
        restore(mask);
        return SYSERR;
    }

    paging_enter_system();
    directory = (pd_t *)pdbr;


    for (index = first_owned; index < PT_ENTRIES; index++) {
        if (directory[index].pd_pres) {
            table_frame = (uint32)directory[index].pd_base << PAGE_SHIFT;
            if (paging_free_pt_frame(table_frame) == SYSERR) {
                panic("Invalid private page-table frame");
            }
        }
    }

    paging_free_pt_frame(pdbr);
    paging_leave_system(previous_pdbr);
    restore(mask);
    return OK;
}
