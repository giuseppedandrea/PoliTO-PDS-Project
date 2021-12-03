#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <clock.h>
#include <syscall.h>
#include <current.h>
#include <lib.h>

#if OPT_SHELL

#include <copyinout.h>
#include <vnode.h>
#include <vfs.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>


#define USE_KERNEL_BUFFER 0


void openfileIncrRefCount(struct openfile *of) {
  if (of!=NULL)
    of->countRef++;
}

/*
 * file system calls for open/close
 */
int sys_open(userptr_t path, int openflags, mode_t mode, int *errp)
{
  int fd, i, ind_table;
  struct vnode *v;
  struct openfile *of=NULL; 	
  int result;

  result = vfs_open((char *)path, openflags, mode, &v);
  if (result) {
    *errp = ENOENT;
    return -1;
  }

  // funzione ritorna indice del vnode nella tabella di sistema oppure errore
  ind_table=sys_fileTable_add(v);
  if(ind_table==-1)
          *errp = ENFILE;
  else {
      fd=proc_fileTable_add(p, ind_table);
      if(fd)
      // no free slot in process open file table
        *errp = EMFILE;
      else
        return fd;     
  }
       
  vfs_close(v);
  return -1;
}

int
sys_close(int fd)
{
  struct openfile *of=NULL; 
  struct vnode *vn;

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  curproc->fileTable[fd] = NULL;

  if (--of->countRef > 0) return 0; // just decrement ref cnt
  vn = of->vn;
  of->vn = NULL;
  if (vn==NULL) return -1;

  vfs_close(vn);	
  return 0;
}

#endif




#if USE_KERNEL_BUFFER

static int
file_read(int fd, userptr_t buf_ptr, size_t size) {
  struct iovec iov;
  struct uio ku;
  int result, nread;
  struct vnode *vn;
  struct openfile *of;
  void *kbuf;

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  vn = of->vn;
  if (vn==NULL) return -1;

  kbuf = kmalloc(size);
  uio_kinit(&iov, &ku, kbuf, size, of->offset, UIO_READ);
  result = VOP_READ(vn, &ku);
  if (result) {
    return result;
  }
  of->offset = ku.uio_offset;
  nread = size - ku.uio_resid;
  copyout(kbuf,buf_ptr,nread);
  kfree(kbuf);
  return (nread);
}

static int
file_write(int fd, userptr_t buf_ptr, size_t size) {
  struct iovec iov;
  struct uio ku;
  int result, nwrite;
  struct vnode *vn;
  struct openfile *of;
  void *kbuf;

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  vn = of->vn;
  if (vn==NULL) return -1;

  kbuf = kmalloc(size);
  copyin(buf_ptr,kbuf,size);
  uio_kinit(&iov, &ku, kbuf, size, of->offset, UIO_WRITE);
  result = VOP_WRITE(vn, &ku);
  if (result) {
    return result;
  }
  kfree(kbuf);
  of->offset = ku.uio_offset;
  nwrite = size - ku.uio_resid;
  return (nwrite);
}

#else

static int
file_read(int fd, userptr_t buf_ptr, size_t size) {
  struct iovec iov;
  struct uio u;
  int result;
  struct vnode *vn;
  struct openfile *of;

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  vn = of->vn;
  if (vn==NULL) return -1;

  iov.iov_ubase = buf_ptr;
  iov.iov_len = size;

  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_resid = size;          // amount to read from the file
  u.uio_offset = of->offset;
  u.uio_segflg =UIO_USERISPACE;
  u.uio_rw = UIO_READ;
  u.uio_space = curproc->p_addrspace;

  result = VOP_READ(vn, &u);
  if (result) {
    return result;
  }

  of->offset = u.uio_offset;
  return (size - u.uio_resid);
}

static int
file_write(int fd, userptr_t buf_ptr, size_t size) {
  struct iovec iov;
  struct uio u;
  int result, nwrite;
  struct vnode *vn;
  struct openfile *of;

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  vn = of->vn;
  if (vn==NULL) return -1;

  iov.iov_ubase = buf_ptr;
  iov.iov_len = size;

  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_resid = size;          // amount to read from the file
  u.uio_offset = of->offset;
  u.uio_segflg =UIO_USERISPACE;
  u.uio_rw = UIO_WRITE;
  u.uio_space = curproc->p_addrspace;

  result = VOP_WRITE(vn, &u);
  if (result) {
    return result;
  }
  of->offset = u.uio_offset;
  nwrite = size - u.uio_resid;
  return (nwrite);
}

#endif


/*
 * simple file system calls for write/read
 */
int
sys_write(int fd, userptr_t buf_ptr, size_t size)
{
  int i;
  char *p = (char *)buf_ptr;

  if (fd!=STDOUT_FILENO && fd!=STDERR_FILENO) {
#if OPT_FILE
    return file_write(fd, buf_ptr, size);
#else
    kprintf("sys_write supported only to stdout\n");
    return -1;
#endif
  }

  for (i=0; i<(int)size; i++) {
    putch(p[i]);
  }

  return (int)size;
}

int
sys_read(int fd, userptr_t buf_ptr, size_t size)
{
  int i;
  char *p = (char *)buf_ptr;

  if (fd!=STDIN_FILENO) {
#if OPT_FILE
    return file_read(fd, buf_ptr, size);
#else
    kprintf("sys_read supported only to stdin\n");
    return -1;
#endif
  }

  for (i=0; i<(int)size; i++) {
    p[i] = getch();
    if (p[i] < 0) 
      return i;
  }

  return (int)size;
}
