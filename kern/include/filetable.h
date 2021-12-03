// vedere se tutta questa roba serve effettivamente
#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include "opt-shell.h"
#include <vnode.h>

#if OPT_SHELL
// qui aggiungere funzioni che agiscono sulla struct definita dentro filetable.c
void sys_fileTable_bootstrap(void);
int sys_fileTable_add(struct vnode *v);

#endif

#endif /* _FILETABLE_H_ */
