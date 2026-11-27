/* kill.c - kill */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  kill  -  Kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
syscall	kill(
	  pid32		pid		/* ID of process to kill	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	int32	i;			/* Index into descriptors	*/
	bool8	self;

	mask = disable();
	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		restore(mask);
		return SYSERR;
	}

	self = (pid == currpid);
	if (self && Defer.ndefers > 0) {
		restore(mask);
		return SYSERR;
	}

	for (i=0; i<3; i++) {
		close(prptr->prdesc[i]);
	}

	/* Finish cleanup before the parent can run after receiving the PID. */
	resched_cntl(DEFER_START);
	send(prptr->prparent, pid);

	if (prptr->prpdbr != system_pdbr) {
		if (self) {
			write_cr3(system_pdbr);
		}
		/* Demand paging will release FFS frames before the tables. */
		if (paging_destroy_user_directory(prptr->prpdbr) == SYSERR) {
			panic("Could not release private page directory");
		}
		freemem((char *)prptr->prvheap_bitmap, VHEAP_BITMAP_SIZE);
	}
	prptr->prpdbr = 0;
	prptr->prvheap_bitmap = NULL;
	freestk(prptr->prstkbase, prptr->prstklen);

	switch (prptr->prstate) {
	case PR_SLEEP:
	case PR_RECTIM:
		unsleep(pid);
		break;

	case PR_WAIT:
		semtab[prptr->prsem].scount++;
		/* Fall through */

	case PR_READY:
		getitem(pid);		/* Remove from queue */
		break;

	default:
		break;
	}
	prptr->prstate = PR_FREE;

	if (--prcount <= 1) {
		xdone();
	}
	if (self) {
		resched();		/* Request a switch when deferral ends. */
	}
	resched_cntl(DEFER_STOP);

	restore(mask);
	return OK;
}
