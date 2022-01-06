#ifndef _ITEM_H_
#define _ITEM_H_

#include "opt-shell.h"
#include <types.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include "filetable.h"
#include "circulararray.h"


#if OPT_SHELL


/* FCB operations */
CAitem newFCB(void);
int cmpFCB(CAkey a , CAkey b);
int freeFCB(CAitem a);
CAitem copyFCB(CAitem source);
CAkey getFCBKey(CAitem source);

/* int operations */
CAitem newInt(void);
int cmpInt(CAkey a , CAkey b);
int freeInt(CAitem a);
CAitem copyInt(CAitem source);
CAkey getIntKey(CAitem source);


#endif

#endif /* _ITEM_H_ */
