#include <types.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <current.h>

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
