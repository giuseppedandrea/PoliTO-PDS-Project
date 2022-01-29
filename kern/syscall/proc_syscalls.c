#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <kern/fcntl.h>
#include <clock.h>
#include <copyinout.h>
#include <vfs.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <mips/trapframe.h>
#include <current.h>
#include <synch.h>

/*
 * _exit syscall - terminate process
 */
void sys__exit(int exitcode)
{
#if OPT_SHELL
  struct proc *proc = curproc;

  /* Set encoded exit status */
  proc->p_exit_status = _MKWAIT_EXIT(exitcode);

  /* Set process has exited */
  proc->p_exited = true;

  /* Remove the current thread from its process */
  proc_remthread(curthread);
  KASSERT(curthread->t_proc == NULL);

  /* Signal for process termination */
  proc_signal(proc);
#else
  (void) status;

  /* get address space of current process and destroy */
  struct addrspace *as = proc_getas();
  as_destroy(as);
#endif
  /* Cause the current thread to exit */
  thread_exit();

  panic("_exit returned (should not happen)\n");
}

#if OPT_SHELL
/*
 * waitpid syscall - wait for a process to exit
 */
pid_t sys_waitpid(pid_t pid, userptr_t statusp, int options, int *errp)
{
  struct proc *proc;
  int result;

  /* Retrieve a process by its pid */
  result = proc_table_search(pid, &proc);
  if (result) {
    *errp = result;
    return -1;
  }

  /* 
   * The only process that is expected to collect another process's exit status is its parent.
   * Check if the pid argument named a process that was not a child of the current process.
   */
  if (curproc->p_pid != proc->p_parent_pid) {
    *errp = ECHILD;
    return -1;
  }

  /* The options argument should be 0 if no options are specified */
  if (options != 0) {
    switch (options) {
      case WNOHANG:
        /*
         * This causes waitpid, when called for a process that has not yet exited,
         * to return 0 immediately instead of waiting.
         */
        if (!proc->p_exited) {
          return 0;
        }
        break;
      default:
        /* The options argument requested invalid or unsupported options */
        *errp = EINVAL;
        return -1;
    }
  }

  /*
   * The status argument can be NULL, in which case waitpid operates normally but the status value is not produced.
   * If the status argument is not NULL, check if it is a valid pointer.
   */
  if ((statusp != NULL)) {
    /*
     * The statusp argument should be an address multiple of 4 (integer alignment)
     * in the address space of the current process.
     */
    if ((((vaddr_t)statusp % 4) != 0) || !as_check_addr(curproc->p_addrspace, (vaddr_t)statusp)) {
      /* The status argument was an invalid pointer */
      *errp = EFAULT;
      return -1;
    }
  }

  /* Wait for process termination, destroy the process, and return exit status */
  result = proc_wait(proc);

  /*
   * If status argument is not NULL, store exit status in the integer pointed to by status argument.
   * Otherwise, waitpid operates normally but the status value is not produced.
   */
  if (statusp != NULL) {
    *(int*)statusp = result;
  }

  return pid;
}

/*
 * getpid syscall - get process ID (pid)
 */
pid_t sys_getpid(void)
{
  pid_t pid;

  KASSERT(curproc != NULL);

  spinlock_acquire(&(curproc->p_lock));
  pid = curproc->p_pid;
  spinlock_release(&(curproc->p_lock));

  return pid;
}

static void call_enter_forked_process(void *tfv, unsigned long dummy) {
  struct trapframe *tf = (struct trapframe *)tfv;

  (void)dummy;

  enter_forked_process(tf);

  panic("call_enter_forked_process returned (should not happen)\n");
}

/*
 * fork syscall - copy the current process
 */
pid_t sys_fork(struct trapframe *ctf, int *errp) {
  struct trapframe *tf_child;
  struct proc *newp;
  int result;

  KASSERT(curproc != NULL);

  /* Create process and set *p_cwd */
  result = proc_create_runprogram(curproc->p_name, &newp);
  if (result) {
    *errp = result;
    return -1;
  }

  /* Done here as we need to duplicate the address space of the current process */
  as_copy(curproc->p_addrspace, &(newp->p_addrspace));
  if(newp->p_addrspace == NULL){
    proc_destroy(newp);
    *errp = ENOMEM;
    return -1;
  }

  proc_file_table_copy(curproc, newp);

  /* We need a copy of the parent's trapframe */
  tf_child = kmalloc(sizeof(struct trapframe));
  if(tf_child == NULL){
    proc_destroy(newp);
    *errp = ENOMEM;
    return -1;
  }
  memcpy(tf_child, ctf, sizeof(struct trapframe));

  result = thread_fork(curthread->t_name, newp,
                       call_enter_forked_process,
                       (void *)tf_child, (unsigned long)0/*unused*/);
  if (result){
    proc_destroy(newp);
    kfree(tf_child);
    *errp = result;
    return -1;
  }

  return newp->p_pid;
}

static void free_kprogname(char *kprogname) {
  kfree(kprogname);
}

static int alloc_kprogname(const char *progname, char **kprogname) {
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

static void free_kargs(int kargc, char **kargs) {
  int i;

  for (i = 0; i < kargc; i++) {
    kfree(kargs[i]);
  }
  kfree(kargs);
}

static int alloc_kargs(char **args, int *kargc, char ***kargs) {
  char **tmp, **_kargs;
  size_t total_size, len;
  int _kargc, n, result;

  KASSERT(curproc != NULL);

  /* Get argc */
  tmp = args;
  _kargc = 0;
  while(*tmp != NULL) {
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

static int copy_kargs(int kargc, char **kargs, vaddr_t *stackptr) {
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
int sys_execv(const char *progname, char **args, int *errp) {
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
#endif
