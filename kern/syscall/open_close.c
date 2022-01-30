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
#include <kern/fcntl.h>


/*
 * file system calls for open/close
 */
int sys_open(userptr_t path, int openflags, mode_t mode, int *errp)
{
  int fd, indTable;
  struct vnode *v;
  int len, result;
  fcb newFile;
  struct proc *p;
  char *kpath;

  p=curproc;

  if(path==NULL || !as_check_addr(curproc->p_addrspace, (vaddr_t)path))
  {
    *errp=EFAULT;
    return -1;
  }

  len = strlen((char *)path)+1;

  kpath=kmalloc(len*sizeof(char));
  if(kpath == NULL) {
      *errp=ENOMEM;
      return -1;
  }
  result = copyin((const_userptr_t)path, kpath, len);
  if(result) {
      *errp=result;
      return -1;
  }


  result = vfs_open((char *)kpath, openflags, mode, &v);
  if (result) {
    *errp = result;
    return -1;
  }

  newFile=newFCB_filled(v, 0, 1, openflags & O_ACCMODE, lock_create("lock_file"));

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
  kfree(kpath);
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

  if(openfileDecrRefCount(file)>0) {
    return 0;
  }
  
  sys_fileTable_remove(proc_fileTable_get(proc, fd));
  proc_fileTable_remove(curproc, fd);

  return 0;
}
