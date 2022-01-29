#include <types.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <current.h>

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
