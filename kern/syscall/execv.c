#include <types.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <vfs.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <current.h>

static void free_kprogname(char *kprogname)
{
  kfree(kprogname);
}

static int alloc_kprogname(const char *progname, char **kprogname)
{
  size_t len;
  char *_kprogname;
  int result;

  /* Allocate kernel memory for progname */
  len = strlen(progname) + 1;
  _kprogname = kmalloc(len * sizeof(*_kprogname));
  if (_kprogname == NULL) {
    return ENOMEM;
  }

  /* Copy progname from user to kernel memory. */
  result = copyin((const_userptr_t)progname, _kprogname, len);
  if (result) {
    /* Free kernel memory allocated for progname. */
    free_kprogname(_kprogname);

    return result;
  }

  *kprogname = _kprogname;

  return 0;
}

static void free_kargs(int kargc, char **kargs)
{
  int i;

  for (i = 0; i < kargc; i++) {
    kfree(kargs[i]);
  }
  kfree(kargs);
}

static int alloc_kargs(char **args, int *kargc, char ***kargs)
{
  char **tmp, **_kargs;
  size_t total_size, len;
  int _kargc, n, result;

  KASSERT(curproc != NULL);

  /* Get argc */
  tmp = args;
  _kargc = 0;
  while (*tmp != NULL) {
    _kargc++;
    tmp++;
  }

  /* Allocate kernel memory for args */
  _kargs = kmalloc((_kargc + 1) * sizeof(*_kargs));
  if (_kargs == NULL) {
    return ENOMEM;
  }

  /* Copy args from user to kernel memory. */
  n = 0;
  _kargs[_kargc] = NULL;
  total_size = sizeof(*_kargs);
  while (args[n] != NULL) {
    /* Check if the argument string is in the address space of the current process. */
    if (!as_check_addr(curproc->p_addrspace, (vaddr_t)args[n])) {
      free_kargs(n, _kargs);

      return EFAULT;
    }

    /* Check if the total size of the argument strings (plus size of pointers) exceeeds ARG_MAX */
    len = strlen(args[n]) + 1;
    total_size += (len + sizeof(*_kargs));
    if (total_size > ARG_MAX) {
      free_kargs(n, _kargs);

      return E2BIG;
    }

    /* Allocate kernel memory for argument string */
    _kargs[n] = kmalloc(len * sizeof(*_kargs));
    if (_kargs[n] == NULL) {
      free_kargs(n, _kargs);

      return ENOMEM;
    }

    /* Copy argument string from user to kernel memory. */
    result = copyin((const_userptr_t)args[n], _kargs[n], len);
    if (result) {
      free_kargs(n + 1, _kargs);

      return result;
    }

    n++;
  }

  *kargc = _kargc;
  *kargs = _kargs;

  return 0;
}

static int copy_kargs(int kargc, char **kargs, vaddr_t *stackptr)
{
  int i, result;
  vaddr_t *kargs_ptrs;
  size_t len, kargs_ptrs_size;
  vaddr_t _stackptr;


  /* Allocate in the kernel memory a temporary array to store argument string poiters. */
  kargs_ptrs_size = (kargc + 1) * sizeof(vaddr_t);
  kargs_ptrs = kmalloc(kargs_ptrs_size);
  if (kargs_ptrs == NULL) {
    return ENOMEM;
  }

  /* Load the argument strings on the stack of the user address space. */
  _stackptr = *stackptr;
  kargs_ptrs[kargc] = 0;
  for (i = kargc - 1; i >= 0; i--) {
    len = strlen(kargs[i]) + 1;
    _stackptr -= len;
    kargs_ptrs[i] = _stackptr;
    result = copyout(kargs[i], (userptr_t)_stackptr, len);
    if (result) {
      kfree(kargs_ptrs);

      return result;
    }
  }

  /*
   * Adjust the stack pointer to be an address multiple of 8 because
   * the largest representable data (double) is 8 bytes.
   */
  _stackptr -= kargs_ptrs_size;
  if (_stackptr % 8) {
    _stackptr -= _stackptr % 8;
  }

  KASSERT((_stackptr % 8) == 0);

  /* Load the argument string pointers on the stack of the user address space. */
  result = copyout(kargs_ptrs, (userptr_t)_stackptr, kargs_ptrs_size);
  if (result) {
    kfree(kargs_ptrs);

    return result;
  }

  /* Free the temporary array to store argument string poiters. */
  kfree(kargs_ptrs);

  *stackptr = _stackptr;

  /*
   * Stack layout of the user address space
   * e.g. `p testbin/add 19 1`
   * ==> argc = 3
   *     argv[0] = "testbin/add"
   *     argv[1] = "19"
   *     argv[2] = "1"
   * +------------------+ <-- stackptr (address multiple of 8)
   * | argv[0] (char *) | ----\
   * +------------------+     |
   * | argv[1] (char *) | --------\
   * +------------------+     |   |
   * | argv[2] (char *) | ------------\
   * +------------------+     |   |   |
   * | NULL             |     |   |   |
   * +------------------+     |   |   |
   * | padding          |     |   |   |
   * +------------------+ <---/   |   |
   * | 't'              |         |   |
   * +------------------+         |   |
   * | 'e'              |         |   |
   * +------------------+         |   |
   * | ...              |         |   |
   * +------------------+         |   |
   * | 'd'              |         |   |
   * +------------------+         |   |
   * | '\0'             |         |   |
   * +------------------+ <-------/   |
   * | '1'              |             |
   * +------------------+             |
   * | '9'              |             |
   * +------------------+             |
   * | '\0'             |             |
   * +------------------+ <-----------/
   * | '1'              |
   * +------------------+
   * | '\0'             |
   * +------------------+
   */


  return 0;
}

/*
 * execv syscall - execute a program
 */
int sys_execv(const char *progname, char **args, int *errp)
{
  char *kprogname;
  char **kargs;
  struct vnode *v;
  struct addrspace *old_as, *new_as;
  vaddr_t entrypoint, stackptr;
  int kargc, result;

  KASSERT(curproc != NULL);

  /*
   * Check if the progname argument is an invalid pointer.
   * The progname argument should be in the address space of the current process.
   */
  if ((progname == NULL) || !as_check_addr(curproc->p_addrspace, (vaddr_t)progname)) {
    *errp = EFAULT;
    return -1;
  }

  /* Allocate kernel memory for progname */
  result = alloc_kprogname(progname, &kprogname);
  if (result) {
    *errp = result;
    return -1;
  }

  /*
   * Check if the args argument is an invalid pointer.
   * The args argument should be in the address space of the current process.
   */
  if ((args == NULL) || !as_check_addr(curproc->p_addrspace, (vaddr_t)args)) {
    *errp = EFAULT;
    return -1;
  }

  /* Allocate kernel memory for args */
  result = alloc_kargs(args, &kargc, &kargs);
  if (result) {
    /* Free kernel memory allocated for progname. */
    free_kprogname(kprogname);

    *errp = result;
    return -1;
  }

  /* Open the program file. */
  result = vfs_open(kprogname, O_RDONLY, 0, &v);
  if (result) {
    /* Free kernel memory allocated for args. */
    free_kargs(kargc, kargs);

    /* Free kernel memory allocated for progname. */
    free_kprogname(kprogname);

    *errp = result;
    return -1;
  }

  /* Save the old address space. */
  old_as = proc_getas();
  KASSERT(old_as != NULL);

  /* Create a new address space. */
  new_as = as_create();
  if (new_as == NULL) {
    /* Close the file. */
    vfs_close(v);

    /* Free kernel memory allocated for args. */
    free_kargs(kargc, kargs);

    /* Free kernel memory allocated for progname. */
    free_kprogname(kprogname);

    *errp = ENOMEM;
    return -1;
  }

  /* Switch to the new address space and activate it. */
  proc_setas(new_as);
  as_activate();

  /* Load the executable. */
  result = load_elf(v, &entrypoint);
  if (result) {
    /* Switch to the old address space, activate it and destroy the new address space. */
    proc_setas(old_as);
    as_activate();
    as_destroy(new_as);

    /* Close the file. */
    vfs_close(v);

    /* Free kernel memory allocated for args. */
    free_kargs(kargc, kargs);

    /* Free kernel memory allocated for progname. */
    free_kprogname(kprogname);

    *errp = result;
    return -1;
  }

  /* Close the program file. */
  vfs_close(v);

  /* Define the user stack in the new address space */
  result = as_define_stack(new_as, &stackptr);
  if (result) {
    /* Switch to the old address space, activate it and destroy the new address space. */
    proc_setas(old_as);
    as_activate();
    as_destroy(new_as);

    /* Free kernel memory allocated for args. */
    free_kargs(kargc, kargs);

    /* Free kernel memory allocated for progname. */
    free_kprogname(kprogname);

    *errp = result;
    return -1;
  }

  /* Copy args on the user stack of the new address space. */
  copy_kargs(kargc, kargs, &stackptr);

  /* Destroy the old address space. */
  as_destroy(old_as);

  /* Free kernel memory allocated for args. */
  free_kargs(kargc, kargs);

  /* Free kernel memory allocated for progname. */
  free_kprogname(kprogname);

  /* Warp to user mode. */
  enter_new_process(kargc /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
    NULL /*userspace addr of environment*/,
    stackptr, entrypoint);

  /* enter_new_process does not return. */
  panic("enter_new_process returned\n");
  *errp = EINVAL;
  return -1;
}
