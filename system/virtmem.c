#include <xinu.h>

#define SYS_PDBR (XINU_PAGES * PAGE_SIZE)

// Page Table/Directory flag masking (top-bottom -> right-left)
uint32 mask_avail     = ((1 << 3) - 1) << 9;
uint32 mask_base      = 0xFFFFFFFF << 12;

// Page index masking (top-bottom -> right-left)
uint32 mask_offset    = (1 << 12) - 1;
uint32 mask_pfn       = ((1 << 20) - 1) << 12;
uint32 mask_pte_idx   = ((1 << 10) - 1) << 12;
uint32 mask_pde_idx   = ((1 << 10) - 1) << 22;

//char* vmalloc(uint32 nbytes){
//    // FIXME
//    return "0";
//}
//
//syscall vfree(char* ptr, uint32 nbytes){
//    // FIXME
//    return OK;
//}

/* ######### HELPERS ######### */

//static void alloc_pde(){
//    // FIXME
//    // NOTE: We can get the address of a pte or page by concatenating the 
//    //       20 PFN bits (found in the PDE) with the PT index (i.e., PT_PFN | PT_IDX).
//}
//
//static void alloc_pte(){
//    // FIXME
//    // NOTE: We can get the address of a pte or page by concatenating the 
//    //       20 PFN bits (found in the PDE) with the PT index (i.e., PT_PFN | PT_IDX).
//}

static void set_pde(
        pd_t* pde,
        uint32 pfn, 
        uint32 valid, 
        uint32 global, 
        uint32 fmb,
        uint32 acc, 
        uint32 pcd, 
        uint32 pwt, 
        uint32 user, 
        uint32 write, 
        uint32 pres
        ){
    pde->pd_base = pfn;
    pde->pd_avail = valid;
    pde->pd_global = global;
    pde->pd_fmb = fmb;
    pde->pd_mbz = 0;
    pde->pd_acc = acc;
    pde->pd_pcd = pcd;
    pde->pd_pwt = pwt;
    pde->pd_user = user;
    pde->pd_write = write;
    pde->pd_pres = pres;
}

static void set_pte(
        pt_t* pte, 
        uint32 pfn, 
        uint32 valid, 
        uint32 global, 
        uint32 dirty, 
        uint32 acc, 
        uint32 pcd, 
        uint32 pwt, 
        uint32 user, 
        uint32 write, 
        uint32 pres
        ){
    pte->pt_base = pfn;
    pte->pt_avail = valid;
    pte->pt_global = global;
    pte->pt_mbz = 0;
    pte->pt_dirty = dirty;
    pte->pt_acc = acc;
    pte->pt_pcd = pcd;
    pte->pt_pwt = pwt;
    pte->pt_user = user;
    pte->pt_write = write;
    pte->pt_pres = pres;
}

//static virt_addr_t set_virt_addr(uint32 address){
//    virt_addr_t vaddr = {0};
//    vaddr.pg_offset = address & mask_offset;
//    vaddr.pt_offset = address & mask_pte_idx;
//    vaddr.pd_offset = address & mask_pde_idx;
//
//    return vaddr;
//}
//
//static phy_addr_t set_phy_addr(uint32 address){
//    phy_addr_t phaddr = {0};
//    phaddr.fm_offset = address & mask_offset;
//    phaddr.fm_num    = (address & mask_pte_idx) + (address & mask_pde_idx);
//
//    return phaddr;
//}

syscall sys_pt_init(){
    pd_t* pd = (pd_t*)SYS_PDBR; // Page Directory base
    pt_t* pt; // Page Table base
    uint32 tot_pde = XINU_PAGES / MAX_PT_SIZE; 
    int i, j; // Loop iterators: i for iterating thorugh PDEs, j for iterating through PTEs
    int k = 0;
    memset(pd, 0, PAGE_SIZE);

    for(i = 0; i < tot_pde; i++){
        pt = (pt_t*)(SYS_PDBR + PAGE_SIZE * (i + 1));
        memset(pt, 0, PAGE_SIZE);
        set_pde(pd + i, (uint32)pt >> 12, 1, 0, 0, 0, 0, 0, 0, 1, 1);
        for(j = 0; j < MAX_PT_SIZE; j++){
            set_pte(pt + j, k++, 1, 0, 0, 0, 0, 0, 0, 1, 1);
        }
    }

    write_cr3(SYS_PDBR);

    return OK;
}
