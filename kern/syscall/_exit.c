#include <types.h>
#include <kern/wait.h>
#include <syscall.h>
#include <proc.h>
#include <thread.h>
#include <current.h>

/*
 * _exit syscall - terminate process
 */
void sys__exit(int exitcode)
{
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

  /* Cause the current thread to exit */
  thread_exit();

  panic("_exit returned (should not happen)\n");
}
