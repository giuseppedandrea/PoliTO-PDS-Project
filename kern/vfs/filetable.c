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
#include "item.h"
#include <spinlock.h>
// OPEN_MAX come limite massimo di file aperti per processo. Sono 128


static cirarray system_openTable; 
static struct lock *sys_openTable_lk;

void openfileIncrRefCount(fcb file) {
  if (file != NULL)
  {
    lock_acquire(file->vn_lk);
    file->countRef++;
    lock_release(file->vn_lk);
  }
}

int openfileDecrRefCount(fcb file) {
  if (file != NULL)
  {
    lock_acquire(file->vn_lk);
    file->countRef--;
    lock_release(file->vn_lk);

    return file->countRef;
  }
  return -1;
}   


void sys_fileTable_bootstrap(void)
{
    CAoperations ops;

    ops.newItem=newFCB;
    ops.cmpItem=cmpFCB;
    ops.freeItem=freeFCB;
    ops.getItemKey=getFCBKey;
    ops.copyItem=copyFCB;
    ops.coutItem=coutFCB;

    system_openTable=CA_create(OPEN_MAX*100, ops);
    sys_openTable_lk=lock_create("sysOpenTable_lk");
}

int sys_fileTable_add(fcb file)
{
    int result;
    if(file==NULL)
        return 0;

    lock_acquire(sys_openTable_lk);
    result=CA_add(system_openTable, file);
    lock_release(sys_openTable_lk);    


    return result;
}

int sys_fileTable_remove(int ind)
{
    int result; 
    
    lock_acquire(sys_openTable_lk);    
    result=CA_remove_byIndex(system_openTable, ind);
    lock_release(sys_openTable_lk);   

    return result;
}

fcb sys_fileTable_get(int ind)
{
    
    fcb file=CA_get_byIndex(system_openTable, ind);
    
    return file;
}

