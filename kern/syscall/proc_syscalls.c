#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <clock.h>
#include <copyinout.h>
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
     * The statusp argument should be an address multiple of 4 (aligned pointer)
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
#endif
