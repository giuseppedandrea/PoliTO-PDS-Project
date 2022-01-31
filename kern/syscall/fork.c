#include <types.h>
#include <kern/errno.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <mips/trapframe.h>
#include <current.h>

static void call_enter_forked_process(void *tfv, unsigned long dummy)
{
  struct trapframe *tf = (struct trapframe *)tfv;

  (void)dummy;

  enter_forked_process(tf);

  panic("call_enter_forked_process returned (should not happen)\n");
}

/*
 * fork syscall - copy the current process
 */
pid_t sys_fork(struct trapframe *ctf, int *errp)
{
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
  if (newp->p_addrspace == NULL) {
    proc_destroy(newp);
    *errp = ENOMEM;
    return -1;
  }

  proc_fileTable_copy(curproc, newp);

  /* We need a copy of the parent's trapframe */
  tf_child = kmalloc(sizeof(struct trapframe));
  if (tf_child == NULL) {
    proc_destroy(newp);
    *errp = ENOMEM;
    return -1;
  }
  memcpy(tf_child, ctf, sizeof(struct trapframe));

  result = thread_fork(curthread->t_name, newp,
    call_enter_forked_process,
    (void *)tf_child, (unsigned long)0/*unused*/);
  if (result) {
    proc_destroy(newp);
    kfree(tf_child);
    *errp = result;
    return -1;
  }

  return newp->p_pid;
}
