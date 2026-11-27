/* paging.h */

#ifndef _PAGING_H_
#define _PAGING_H_

/* Macros */
#define XINU_PAGES      8192    /* number of pages used by default by Xinu               */
#define PAGE_SIZE       4096    /* number of bytes per page                              */
#define MAX_FFS_SIZE    16*1024 /* size of FFS space  (in frames)                        */
#define MAX_PT_SIZE     1024    /* size of space used for page tables (in frames)        */
#define PT_ENTRIES      1024U
#define PAGE_SHIFT      12U
#define VIRTUAL_PAGE_COUNT (PT_ENTRIES * PT_ENTRIES)
#define PAGEFAULT_VECTOR 14U
/* Physical FFS frame numbers start immediately past the page-table pool. */
#define FFS_FIRST_FRAME ((XINU_PAGES) + (MAX_PT_SIZE))


/* Structure for a page directory entry */

typedef struct {
 
  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;



// int x;
// pd_t  pde = 


/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

/* Structure for a virtual address */

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

/* Structure for a physical address */

typedef struct{
  unsigned int fm_offset : 12;		/* frame offset			*/
  unsigned int fm_num : 20;		/* frame number			*/
} phy_addr_t;

/* Functions to manipulate control registers and enable paging (see control_reg.c)	 */

unsigned long read_cr0(void);

unsigned long read_cr2(void);

unsigned long read_cr3(void);

unsigned long read_cr4(void);

void write_cr0(unsigned long n);

void write_cr3(unsigned long n);

void write_cr4(unsigned long n);

void enable_paging(void);

pd_t *paging_build_system(void);

extern uint32 system_pdbr;

// The below are added while creating Vcreate and allocating
// the directory structure for a user process

#define VHEAP_BASE ((uint32)(XINU_PAGES) * PAGE_SIZE)
#define VHEAP_PAGE_COUNT (VIRTUAL_PAGE_COUNT - (XINU_PAGES))

#define VHEAP_BITMAP_SIZE ((VHEAP_PAGE_COUNT + 7U) / 8U )

/* Keep interrupts disabled while borrowing the system directory. */
uint32 paging_enter_system(void);
void paging_leave_system(uint32 previous_pdbr);

/* Keep the system directory active and interrupts disabled for pool edits. */
uint32 paging_alloc_pt_frame(void);
status paging_free_pt_frame(uint32 frame);

uint32 paging_create_user_directory(void);
status paging_destroy_user_directory(uint32 pdbr);

void pagefault_handler_disp(void);
void pagefault_handler(void);

/* Called with interrupts disabled after checking the page is reserved. */
status paging_map_heap_page(uint32 address);

/* Release FFS mappings for a heap-relative range, keeping the page tables. */
status paging_release_heap_pages(pid32 pid, uint32 first_page, uint32 npages);
uint32 free_ffs_pages(void);
uint32 used_ffs_frames(pid32 pid);
uint32 allocated_virtual_pages(pid32 pid);


#endif
