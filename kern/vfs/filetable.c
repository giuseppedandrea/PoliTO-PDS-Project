#include <types.h>
#include <kern/errno.h>
#include <kern/reboot.h>
#include <kern/unistd.h>
#include <lib.h>
#include <spl.h>
#include <clock.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vm.h>
#include <mainbus.h>
#include <vfs.h>
#include <device.h>
#include <syscall.h>
#include <test.h>
#include <version.h>
#include "autoconf.h"  // for pseudoconfig
#include "filetable.h"
#include "opt-shell.h"

// OPEN_MAX come limite massimo di file aperti per processo. Sono 128

typedef struct _file fcb;

static struct _syswideOpenTable system_openTable;

struct _syswideOpenTable{
    fcb *table;
    int dim_table;
};

struct _file
{
    struct vnode *vn;
    off_t offset; 
    unsigned int countRef;
    struct lock vn_lk;
};  

void sys_fileTable_bootstrap(void)
{

    system_openTable.dim_table=50; // valore iniziale tabella di file, 
                                   // eventualmente da aggiornare
    system_openTable.table=(fcb *) kmalloc(system_openTable.dim_table*sizeof(fcb));
}

int sys_fileTable_add(struct vnode *v)
{
    int i, ind=-1;

    for (i=0; i<system_openTable.dim_table; i++) {
    if (system_openTable.table[i].vn==NULL) {
        ind=i;
        system_openTable.table[i].vn = v;
        system_openTable.table[i].offset = 0; // TODO: handle offset with append
        system_openTable.table[i].countRef = 1;
        break;
        }
    }

    if(ind==-1)
        if(system_openTable.dim_table<MAX_OPEN_TABLE)
        {
            // rialloco
        }
    
    return ind;
}

