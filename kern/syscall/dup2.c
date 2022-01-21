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
    int indTable;

    if(oldfd<0 || oldfd>OPEN_MAX || newfd<0 || newfd>OPEN_MAX) {
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
        if(sys_close(newfd, errp)!=0) {
            *errp=EBADF;
            return -1;
        }
        freeFCB(newFile);
    }

    newFile=copyFCB(fileToCpy);
    indTable=sys_fileTable_add(newFile);
    if(indTable==0)
        *errp = ENFILE;
    else {
        // qui serve una SET nel circulararray. IMplementare di corsa
        fd=proc_fileTable_add(curproc, indTable); 
        if(fd==0)
        // no free slot in process open file table
            *errp = EMFILE;
        else
            return fd;     
    }
       


}
