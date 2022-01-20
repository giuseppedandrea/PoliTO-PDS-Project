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
#include "circulararray.h"
#include "item.h"

// OPEN_MAX come limite massimo di file aperti per processo. Sono 128



static cirarray system_openTable; 


void openfileIncrRefCount(fcb file) {
  if (file != NULL)
    file->countRef++;
}   


void sys_fileTable_bootstrap(void)
{
    CAoperations ops;

    ops.newItem=newFCB;
    ops.cmpItem=cmpFCB;
    ops.freeItem=freeFCB;
    ops.getItemKey=getFCBKey;
    ops.copyItem=copyFCB;

    system_openTable=CA_create(OPEN_MAX*100, ops);
}

int sys_fileTable_add(fcb file)
{
    if(file==NULL)
        return 0;
        
    return CA_add(system_openTable, file);
}

int sys_fileTable_remove(int ind)
{
    return CA_remove_byIndex(system_openTable, ind);
}

fcb sys_fileTable_get(int ind)
{
    fcb file=CA_get_byIndex(system_openTable, ind);
    return file;
}

