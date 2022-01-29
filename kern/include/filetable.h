// vedere se tutta questa roba serve effettivamente
#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include "opt-shell.h"
#include <vnode.h>

#if OPT_SHELL

typedef struct _file* fcb;

struct _file
{
    struct vnode *vn;
    off_t size;
    off_t offset;
    int flag; 
    unsigned int countRef;
    struct lock *vn_lk;
    
};  


void openfileIncrRefCount(fcb file);

// qui aggiungere funzioni che agiscono sulla struct definita dentro filetable.c
void sys_fileTable_bootstrap(void);
// returns 0 on error or fd
int sys_fileTable_add(fcb file);
// returns 1 on error. 0 on success
int sys_fileTable_remove(int ind);
// returns fcb if present or null
fcb sys_fileTable_get(int ind);

#endif

#endif /* _FILETABLE_H_ */
