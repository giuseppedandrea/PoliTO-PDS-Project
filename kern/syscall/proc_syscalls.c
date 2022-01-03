#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
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
 * system calls for process management
 */
void sys__exit(int status)
{
#if OPT_SHELL
  // probably here must wait on semaphone due to cuncurrent access to 
  // a critical section (curproc)
  // SEGUIRE TRAIETTORIA CONCORRENZA PROCESSI
  struct proc *p = curproc;
  p->p_status = status & 0xff; /* just lower 8 bits returned */
  proc_remthread(curthread);
  proc_signal_end(p);
#else
  (void) status;

  /* get address space of current process and destroy */
  struct addrspace *as = proc_getas();
  as_destroy(as);
#endif
  thread_exit();

  panic("thread_exit returned (should not happen)\n");
}

#if OPT_SHELL
pid_t sys_waitpid(pid_t pid, userptr_t statusp, int options, int *errp)
{
  struct proc *p;
  int s;

  (void)options; /* not handled */

  p = proc_search_pid(pid);
  if (p == NULL) {
    // TODO: Error handling
    (void)errp;
    return -1;
  }

  s = proc_wait(p);
  if (statusp != NULL) {
    *(int*)statusp = s;
  } else {
    *errp = EFAULT;
    return -1;
  }

  return pid;
}
#endif

pid_t sys_getpid(void)
{
#if OPT_SHELL
  struct proc *p;

  KASSERT(curproc != NULL);

  spinlock_acquire(&(curproc->p_lock));
  p=curproc;
  spinlock_release(&(curproc->p_lock));

  return p->p_pid;
#else
  return -1;
#endif
}

#if OPT_SHELL
static void call_enter_forked_process(void *tfv, unsigned long dummy) {
  struct trapframe *tf = (struct trapframe *)tfv;

  (void)dummy;

  enter_forked_process(tf);

  panic("enter_forked_process returned (should not happen)\n");
}

pid_t sys_fork(struct trapframe *ctf, int *errp) {
  struct trapframe *tf_child;
  struct proc *newp;
  int result;

  KASSERT(curproc != NULL);

  newp = proc_create_runprogram(curproc->p_name); // create process and set *p_cwd
  if (newp == NULL) {
    *errp = ENOMEM;
    return -1;
  }

  /* done here as we need to duplicate the address space 
     of the current process */
  as_copy(curproc->p_addrspace, &(newp->p_addrspace));
  if(newp->p_addrspace == NULL){
    proc_destroy(newp);
    *errp = ENOMEM;
    return -1;
  }

  proc_file_table_copy(curproc, newp);

  /* we need a copy of the parent's trapframe */
  tf_child = kmalloc(sizeof(struct trapframe));
  if(tf_child == NULL){
    proc_destroy(newp);
    *errp = ENOMEM;
    return -1;
  }
  memcpy(tf_child, ctf, sizeof(struct trapframe));

  /* link child process to its parent, so that child terminates on parent exit */
  result = procChild_add(curproc, newp);
  if(result){
    proc_destroy(newp);
    kfree(tf_child);
    *errp = result;
    return -1;
  }

  result = thread_fork(
		 curthread->t_name, newp,
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
