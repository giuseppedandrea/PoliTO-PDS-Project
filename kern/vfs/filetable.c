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
    off_t offset; // probabilmente l'offset appartiene alle informazioni che un processo possiede per file, quindi informazione verrà spostata
    unsigned int countRef;
    struct lock vn_lk;
};  

void fileTable_bootstrap(void)
{

    system_openTable.dim_table=50; // valore iniziale tabella di file, 
                                   // eventualmente da aggiornare
    system_openTable.table=(fcb *) kmalloc(system_openTable.dim_table*sizeof(fcb));
}
