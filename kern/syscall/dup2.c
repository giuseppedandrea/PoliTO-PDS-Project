#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <clock.h>
#include <syscall.h>
#include <current.h>
#include <lib.h>
#include <copyinout.h>
#include <vnode.h>
#include <vfs.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>
#include "item.h"

int sys_dup2(int oldfd, int newfd, int *errp)
{
    fcb fileToCpy, newFile;
    int indTable, result;

    if(oldfd<0 || oldfd>OPEN_MAX || newfd<0 || newfd>OPEN_MAX || oldfd==newfd) {
        *errp=EBADF;
        return -1;
    }

    fileToCpy=sys_fileTable_get(proc_fileTable_get(curproc, oldfd));
    if(fileToCpy==NULL) {
        *errp=EBADF;
        return -1;
    }

    newFile=sys_fileTable_get(proc_fileTable_get(curproc, newfd));
    if(newFile!=NULL)
    {
        sys_fileTable_remove(proc_fileTable_get(curproc, newfd));
        proc_fileTable_remove(curproc, newfd);
        freeFCB(newFile);
    }

    newFile=copyFCB(fileToCpy);
    indTable=sys_fileTable_add(newFile);
    if(indTable==0)
        *errp = ENFILE;
    else 
    {
        result=proc_fileTable_set(curproc, indTable, newfd); 
        if(result)
            // no free slot in process open file table
            *errp = EMFILE;
        else 
        {
            return newfd;
        }     
    }
    
    return -1;
}
