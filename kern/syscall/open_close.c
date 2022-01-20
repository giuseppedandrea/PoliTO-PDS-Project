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


/*
 * file system calls for open/close
 */
int sys_open(userptr_t path, int openflags, mode_t mode, int *errp)
{
  int fd, indTable;
  struct vnode *v;
  int result;
  fcb newFile;
  struct proc *p;

  p=curproc;

  if((char *) path == NULL)
  {
    *errp=EFAULT;
    return -1;
  }

  result = vfs_open((char *)path, openflags, mode, &v);
  if (result) {
    *errp = result;
    return -1;
  }

  newFile=newFCB_filled(v, 0, 1, lock_create("lock_file"));

  // funzione ritorna indice del vnode nella tabella di sistema oppure errore
  indTable=sys_fileTable_add(newFile);
  if(indTable==0)
      *errp = ENFILE;
  else {
      fd=proc_fileTable_add(p, indTable);
      if(fd==0)
      // no free slot in process open file table
        *errp = EMFILE;
      else
        return fd;     
  }
       
  vfs_close(v);
  return -1;
}

int sys_close(int fd, int *errp)
{
  fcb file=NULL; 
  struct proc *proc;

  if(fd < 0 || fd > OPEN_MAX) 
  {
    *errp=EBADF; 
    return -1;
  }
  
  proc=curproc;

  file=sys_fileTable_get(proc_fileTable_get(proc, fd));
  if(file==NULL)
  {
    *errp=EBADF; 
    return -1;
  }
  if(--file->countRef > 0) 
    return 0; // just decrement ref cnt
  
  sys_fileTable_remove(proc_fileTable_get(proc, fd));
  proc_fileTable_remove(curproc, fd);

  return 0;
}
