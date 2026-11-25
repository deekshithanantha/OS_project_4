#include <xinu.h>

char* xinu_pdbr;
char* xinu_end;
uint32 xinu_space;

uint32 mask_pres      = 1;
uint32 mask_write     = 1 << 1;
uint32 mask_user      = 1 << 2;
uint32 mask_pwt       = 1 << 3;
uint32 mask_pcd       = 1 << 4;
uint32 mask_acc       = 1 << 5;
uint32 mask_pd_mbz    = 1 << 6;
uint32 mask_dirty     = 1 << 6;
uint32 mask_pt_mbz    = 1 << 7;
uint32 mask_fmb       = 1 << 7;
uint32 mask_global    = 1 << 8;
uint32 mask_avail     = 7 << 11;
uint32 mask_base      = 0xFFFFFFFF << 12;

char* vmalloc(uint32 nbytes){
    // FIXME
    return "0";
}

syscall vfree(char* ptr, uint32 nbytes){
    // FIXME
    return OK;
}

/* ######### HELPERS ######### */

static void set_pde(pd_t* pde, virt_addr_t vaddr){
    // FIXME: Need to figure out what to set each bit to.
    pde.pd_pres     = 1;
    pde.pd_write    = 0;
    pde.pd_user     = 0;
    pde.pd_pwt      = 0;
    pde.pd_pcd      = 0;
    pde.pd_acc      = 0;
    pde.pd_mbz      = 0;
    pde.pd_fmb      = 0;
    pde.pd_global   = 0;
    pde.pd_avail    = 0;
    pde.pd_base     = vaddr.fm_num & base_mask;
}

static void set_pte(pt_t* pt, virt_addr_t vaddr){
    // FIXME: Need to figure out what to set each bit to.
    pde.pd_pres     = 1;
    pde.pd_write    = 0;
    pde.pd_user     = 0;
    pde.pd_pwt      = 0;
    pde.pd_pcd      = 0;
    pde.pd_acc      = 0;
    pde.pd_dirty    = 0;
    pde.pd_mbz      = 0;
    pde.pd_global   = 0;
    pde.pd_avail    = 0;
    pde.pd_base     = vaddr.fm_num & base_mask;
}

static virt_addr_t set_virt_addr(char* address){
    virt_addr_t vaddr;
    // FIXME: Need to finish implenting how the virtual address gets set.

    return vaddr;
}

static phy_addr_t set_phy_addr(char* address){
    phy_addr_t phaddr;
    // FIXME: Need to finish implenting how the physical address gets set.

    return phaddr;
}

syscall sys_pt_init(){
    xinu_space = XINU_PAGES * PAGE_SIZE;
    xinu_end = (char*)(xinu_space - 1);
    xinu_pdbr = (char*)xinu_space;
    pd_t* pd = (pd_t*)xinu_pdbr;

    uint32 tot_pde = XINU_PAGES / MAX_PT_SIZE;
    int i;
    pd_t pde;
    for(i = 0; i < tot_pde; i++){
        // FIXME: Need to actually have the correct typing passed.
        virt_addr_t vaddr = set_virt_addr(xinu_pdbr + i + 1); 
        set_pde(pd[i], vaddr);
        // FIXME: When do I actually call to set PTEs?
    }

    write_cr3(pde);

    return OK;
}
