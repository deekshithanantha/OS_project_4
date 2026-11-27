/* create.c - create, vcreate, newpid */

#include <xinu.h>

#define INITIAL_STACK_WORDS 13U

local pid32 newpid(void);

static pid32 create_process(void *funcaddr, uint32 ssize, pri16 priority,
                            char *name, uint32 nargs, const void *args,
                            bool8 private_heap)
{
    uint32 savsp, *pushsp, *saddr;
    intmask mask;
    pid32 pid;
    struct procent *prptr;
    uint32 pdbr = system_pdbr;
    byte *heap_bitmap = NULL;
    int32 i;

    mask = disable();
    if (ssize < MINSTK) {
        ssize = MINSTK;
    }
    if (priority < 1 || ssize > 0xFFFFFFFFU - 7U) {
        restore(mask);
        return SYSERR;
    }
    ssize = (uint32)roundmb(ssize);
    if (nargs > ssize / sizeof(uint32) - INITIAL_STACK_WORDS ||
        (pid = newpid()) == SYSERR ||
        (saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) {
        restore(mask);
        return SYSERR;
    }

    if (private_heap) {
        /* The bitmap records reservations. Heap frames are allocated later. */
        heap_bitmap = (byte *)getmem(VHEAP_BITMAP_SIZE);
        if (heap_bitmap == (byte *)SYSERR) {
            goto free_stack;
        }

        pdbr = paging_create_user_directory();
        if (pdbr == 0) {
            freemem((char *)heap_bitmap, VHEAP_BITMAP_SIZE);
            goto free_stack;
        }
        memset(heap_bitmap, 0, VHEAP_BITMAP_SIZE);
    }

    prptr = &proctab[pid];
    prptr->prpdbr = pdbr;
    prptr->prvheap_bitmap = heap_bitmap;
    prptr->prprio = priority;
    prptr->prstkbase = (char *)saddr;
    prptr->prstklen = ssize;
    prptr->prname[PNMLEN - 1] = NULLCH;
    for (i = 0; i < PNMLEN - 1 && (prptr->prname[i] = name[i]) != NULLCH; i++) {
        ;
    }
    prptr->prsem = -1;
    prptr->prparent = (pid32)getpid();
    prptr->prhasmsg = FALSE;
    prptr->prdesc[0] = CONSOLE;
    prptr->prdesc[1] = CONSOLE;
    prptr->prdesc[2] = CONSOLE;

    *saddr = STACKMAGIC;
    savsp = (uint32)saddr;

    /* The i386 argument words appear in call order above the return address. */
    if (nargs != 0) {
        saddr -= nargs;
        memcpy(saddr, args, nargs * sizeof(uint32));
    }
    *--saddr = (uint32)INITRET;

    /* Match the stack layout that ctxsw restores on its first switch. */
    *--saddr = (uint32)funcaddr;
    *--saddr = savsp;
    savsp = (uint32)saddr;
    *--saddr = 0x00000200;       /* Start with interrupts enabled. */

    *--saddr = 0;                /* eax */
    *--saddr = 0;                /* ecx */
    *--saddr = 0;                /* edx */
    *--saddr = 0;                /* ebx */
    *--saddr = 0;                /* esp, filled in below */
    pushsp = saddr;
    *--saddr = savsp;            /* ebp */
    *--saddr = 0;                /* esi */
    *--saddr = 0;                /* edi */
    prptr->prstkptr = (char *)saddr;
    *pushsp = (uint32)saddr;

    prptr->prstate = PR_SUSP;
    prcount++;
    restore(mask);
    return pid;

free_stack:
    freestk((char *)saddr, ssize);
    restore(mask);
    return SYSERR;
}

pid32 create(void *funcaddr, uint32 ssize, pri16 priority, char *name,
             uint32 nargs, ...)
{
    return create_process(funcaddr, ssize, priority, name, nargs,
                          __builtin_next_arg(nargs), FALSE);
}

pid32 vcreate(void *funcaddr, uint32 ssize, pri16 priority, char *name,
              uint32 nargs, ...)
{
    return create_process(funcaddr, ssize, priority, name, nargs,
                          __builtin_next_arg(nargs), TRUE);
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}
	return (pid32) SYSERR;
}
