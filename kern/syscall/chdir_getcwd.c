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

int sys_chdir(const char *pathname, int *errp)
{
  int len, result;
  char *kpath;

  if (pathname == NULL || !as_check_addr(curproc->p_addrspace, (vaddr_t)pathname)) {
    *errp = EFAULT;
    return -1;
  }

  len = strlen(pathname) + 1;

  kpath = kmalloc(len * sizeof(char));
  if (kpath == NULL) {
    *errp = ENOMEM;
    return -1;
  }
  result = copyin((const_userptr_t)pathname, kpath, len);
  if (result) {
    *errp = result;
    return -1;
  }

  result = vfs_chdir(kpath);
  if (result) {
    *errp = result;
    return -1;
  }

  kfree(kpath);

  return 0;
}

int sys___getcwd(char *ptr, size_t bufflen, int *errp)
{
  struct iovec iov;
  struct uio u;
  int result;

  if (ptr == NULL || !as_check_addr(curproc->p_addrspace, (vaddr_t)ptr)) {
    *errp = EFAULT;
    return -1;
  }

  iov.iov_ubase = (userptr_t)ptr;
  iov.iov_len = bufflen;
  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_resid = bufflen;          // amount to read from the file
  u.uio_offset = 0;
  u.uio_segflg = UIO_USERISPACE;
  u.uio_rw = UIO_READ;
  u.uio_space = curproc->p_addrspace;

  result = vfs_getcwd(&u);
  if (result) {
    *errp = result;
    return -1;
  }

  return bufflen - u.uio_offset;
}
