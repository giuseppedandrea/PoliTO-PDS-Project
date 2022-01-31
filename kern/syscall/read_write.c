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

static int file_write(int fd, userptr_t buf_ptr, size_t size, int *errp)
{
  struct iovec iov;
  struct uio u;
  int result, nwrite;
  struct vnode *vn;
  fcb file;
  struct proc *p;
  struct stat *st = kmalloc(sizeof(struct stat));

  if (fd<0 || fd>OPEN_MAX) {
    *errp = EBADF;
    return -1;
  }

  if (buf_ptr == NULL || !as_check_addr(curproc->p_addrspace, (vaddr_t)buf_ptr)) {
    *errp = EFAULT;
    return -1;
  }

  p = curproc;

  //kprintf("Tabella processi: \n");
  //CA_stamp(p->ft);

  file = sys_fileTable_get(proc_fileTable_get(p, fd));
  if (file == NULL || file->flag == O_RDONLY) {
    *errp = EBADF;
    return -1;
  }

  vn = file->vn;
  iov.iov_ubase = buf_ptr;
  iov.iov_len = size;

  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_resid = size;          // amount to read from the file


  // try to implement a primitive solution.
  lock_acquire(file->vn_lk);

  u.uio_offset = file->offset;

  lock_release(file->vn_lk);

  u.uio_segflg = UIO_USERISPACE;
  u.uio_rw = UIO_WRITE;
  u.uio_space = p->p_addrspace;

  result = VOP_WRITE(vn, &u);
  if (result) {
    *errp = result;
    return -1;
  }

  lock_acquire(file->vn_lk);
  VOP_STAT(file->vn, st);
  file->size = st->st_size;;
  lock_release(file->vn_lk);

  file->offset = u.uio_offset;
  nwrite = size - u.uio_resid;

  return (nwrite);
}

int sys_write(int fd, userptr_t buf_ptr, size_t size, int *errp)
{
  int i;
  char *p = (char *)buf_ptr;

  if (fd != STDOUT_FILENO && fd != STDERR_FILENO)
    return file_write(fd, buf_ptr, size, errp);

  for (i = 0; i < (int)size; i++) {
    putch(p[i]);
  }

  return (int)size;
}


static int file_read(int fd, userptr_t buf_ptr, size_t size, int *errp)
{
  struct iovec iov;
  struct uio u;
  int result;
  struct vnode *vn;
  fcb file;
  struct proc *p;
  int nread;
  struct stat st;

  if (fd<0 || fd>OPEN_MAX) {
    *errp = EBADF;
    return -1;
  }

  if (buf_ptr == NULL || !as_check_addr(curproc->p_addrspace, (vaddr_t)buf_ptr)) {
    *errp = EFAULT;
    return -1;
  }

  p = curproc;

  file = sys_fileTable_get(proc_fileTable_get(p, fd));
  if (file == NULL || file->flag == O_WRONLY) {
    *errp = EBADF;
    return -1;
  }

  vn = file->vn;
  VOP_STAT(vn, &st);

  iov.iov_ubase = buf_ptr;
  iov.iov_len = size;

  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_resid = size;          // amount to read from the file
  u.uio_offset = file->offset;
  u.uio_segflg = UIO_USERISPACE;
  u.uio_rw = UIO_READ;
  u.uio_space = p->p_addrspace;

  result = VOP_READ(vn, &u);
  if (result) {
    *errp = result;
    return -1;
  }

  file->offset = u.uio_offset;
  nread = size - u.uio_resid;
  return nread;
}


int sys_read(int fd, userptr_t buf_ptr, size_t size, int *errp)
{
  int i;
  char *buff = (char *)buf_ptr;


  if (fd != STDIN_FILENO) {
    return file_read(fd, buf_ptr, size, errp);
  }

  for (i = 0; i < (int)size; i++) {
    buff[i] = getch();
    if (buff[i] < 0)
      return i;
  }

  return (int)size;
}
