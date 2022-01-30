/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Process support.
 *
 * There is (intentionally) not much here; you will need to add stuff
 * and maybe change around what's already present.
 *
 * p_lock is intended to be held when manipulating the pointers in the
 * proc structure, not while doing any significant work with the
 * things they point to. Rearrange this (and/or change it to be a
 * regular lock) as needed.
 *
 * Unless you're implementing multithreaded user processes, the only
 * process that will have more than one thread is the kernel process.
 */

#include <types.h>
#include <kern/errno.h>
#include <spl.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <syscall.h>
#include "item.h"
#include <synch.h>
#include <limits.h>
#include <kern/unistd.h>

/* Max number of active processes on the system */
#define MAX_SYSTEM_PROCS 1024

/* Max number of child processes refers to a single process */
#define MAX_CHILD_PROCS 100

/*
 * The process for the kernel; this holds all the kernel-only threads.
 */
struct proc *kproc;

#if OPT_SHELL
struct proc_table {
  /*
   * array of current active processes
   * [0] and [1] are used for kernel process
   * user processes have a pid >= PID_MIN
   */
  struct proc **procs;
  /* number of current active processes */
  size_t numprocs;
  /* next available pid */
  size_t nextpid;
  /* lock for this struct */
  struct spinlock lock;
};

struct proc_table *pt;

#define PROCS_IDX(X) ((X) < MAX_SYSTEM_PROCS ? (X) : PID_MIN + ((X - MAX_SYSTEM_PROCS) % (MAX_SYSTEM_PROCS - PID_MIN)))

/*
 * Create the Process Table.
 * Return proper error code on error:
 * - ENOMEM: sufficient memory was not available
 */
static int proc_table_create()
{
      size_t i;

      pt = kmalloc(sizeof(*pt));
      if (pt == NULL) {
        return ENOMEM;
      }

      pt->procs = kmalloc(MAX_SYSTEM_PROCS * sizeof(*(pt->procs)));
      if (pt->procs == NULL) {
        return ENOMEM;
      }

      spinlock_init(&pt->lock);
      pt->numprocs = 0;
      pt->nextpid = PID_MIN;

      for (i = 0; i < MAX_SYSTEM_PROCS; i++) {
        pt->procs[i] = NULL;
      }

      return 0;
}

/*
 * Add a process to the Process Table.
 * Return proper error code on error:
 * - ENPROC: there are already too many processes on the system
 */
static int proc_table_add(struct proc *proc)
{
      pid_t pid;

      KASSERT(proc != NULL);
      KASSERT(pt != NULL);

      spinlock_acquire(&pt->lock);

      /* Check if there are already too many processes on the system */
      if (pt->numprocs >= (MAX_SYSTEM_PROCS - PID_MIN)) {
        spinlock_release(&pt->lock);
        return ENPROC;
      }

      /* Check if a wrap around is needed for next possible pid */
      if (pt->nextpid > PID_MAX) {
        pt->nextpid = PID_MIN;
      }

      /* Looking for a free slot in array of current active processes */
      pid = pt->nextpid++;
      while (pt->procs[PROCS_IDX(pid)] != NULL) {
        pid = pt->nextpid++;
        /* Check if a wrap around is needed for next possible pid */
        if (pt->nextpid > PID_MAX) {
          pt->nextpid = PID_MIN;
        }
      }

      /* Assign pid to process */
      proc->p_pid = pid;

      /* Store proc pointer in array of current active processes */
      pt->procs[PROCS_IDX(pid)] = proc;

      /* Increment number of current active processes */
      pt->numprocs++;

      KASSERT(pt->numprocs <= (MAX_SYSTEM_PROCS - PID_MIN));

      spinlock_release(&pt->lock);

      return 0;
}

/*
 * Remove a process from the Process Table.
 */
static void proc_table_remove(struct proc *proc)
{
      pid_t pid;
      struct proc* p;

      KASSERT(proc != NULL);
      KASSERT(pt != NULL);

      pid = proc->p_pid;

      KASSERT((pid >= PID_MIN) && (pid <= PID_MAX));

      spinlock_acquire(&pt->lock);

      /* Retrieve proc pointer from array of current active processes */
      p = pt->procs[PROCS_IDX(pid)];
      if (p != NULL) {
        /* Free corresponding slot in array of current active processes */
        pt->procs[PROCS_IDX(pid)] = NULL;
        /* Decrement number of current active processes */
        pt->numprocs--;
      }

      spinlock_release(&pt->lock);
}

/*
 * Create the list of child processes.
 * Return proper error code on error:
 * - ENOMEM: sufficient memory was not available
 */
static int proc_children_create(struct proc *proc)
{
      KASSERT(proc != NULL);

      proc->p_children = list_create();
      if (proc->p_children == NULL) {
        return ENOMEM;
      }

      return 0;
}

/*
 * Destroy the list of child processes.
 */
static void proc_children_destroy(struct proc *proc)
{
      KASSERT(proc != NULL);

      KASSERT(proc->p_children != NULL);
      KASSERT(list_isEmpty(proc->p_children));

      list_destroy(proc->p_children);
      proc->p_children = NULL;
}

/*
 * Add a process to the list of child processes.
 * Return proper error code on error:
 * - ENOMEM: sufficient memory was not available
 * - EMPROC: parent process has too much children processes
 */
static int proc_children_add(struct proc *pparent, struct proc *pchild)
{
      int result;

      KASSERT(pparent != NULL);
      KASSERT(pchild != NULL);

      KASSERT(pchild != kproc);

      pchild->p_parent_pid = pparent->p_pid;

      if (list_size(pparent->p_children) < MAX_CHILD_PROCS) {
        /* Add the child process to the list of children of the parent process */
        result = list_insertTail(pparent->p_children, pchild);
        if (result) {
          return result;
        }
      } else {
        /* Parent process has too much children processes */
        return EMPROC;
      }

      return 0;
}

/*
 * Remove a process from the list of child processes.
 */
static void proc_children_remove(struct proc *proc)
{
      struct proc *pparent, *pchild;

      KASSERT(proc != NULL);

      pparent = pt->procs[PROCS_IDX(proc->p_parent_pid)];

      KASSERT(pparent != NULL);

      /* Remove the child process from the list of children of the parent process */
      pchild = list_deleteByKey(pparent->p_children, &(proc->p_pid), (unsigned char *)(&(proc->p_pid)) - (unsigned char *)proc, sizeof(proc->p_pid));
      KASSERT(proc == pchild);
}

/*
 * Creating file table of single process.
 */
static int proc_fileTable_create(struct proc *proc)
{
  CAoperations ops;
  int *stdin, *stdout, *stderr;

  KASSERT(proc != NULL);

  ops.newItem=newInt;
  ops.cmpItem=cmpInt;
  ops.freeItem=freeInt;
  ops.copyItem=copyInt;
  ops.getItemKey=getIntKey;
  ops.coutItem=coutInt;

  proc->ft=CA_create(OPEN_MAX, ops);
  if(proc->ft==NULL)
    return ENOMEM;

  stdin=newInt();
  stdout=newInt();
  stderr=newInt();

  CA_add(proc->ft, stdin);
  CA_add(proc->ft, stdout);
  CA_add(proc->ft, stderr);

  return 0;
}

/*
 * Destroy file table of single process.
 */
static int proc_fileTable_destroy(struct proc *proc)
{
  KASSERT(proc != NULL);

  return CA_destroy(proc->ft);
}
#endif

/*
 * Create a proc structure.
 * Return proper error code on error:
 * - ENOMEM: sufficient memory was not available
 * - ENPROC: there are already too many processes on the system
 */
static int proc_create(const char *name, struct proc **retproc)
{
      struct proc *proc;

      proc = kmalloc(sizeof(*proc));
      if (proc == NULL) {
        return ENOMEM;
      }
      proc->p_name = kstrdup(name);
      if (proc->p_name == NULL) {
        kfree(proc);
        return ENOMEM;
      }

      spinlock_init(&proc->p_lock);
      proc->p_numthreads = 0;

      /* VM fields */
      proc->p_addrspace = NULL;

      /* VFS fields */
      proc->p_cwd = NULL;

#if OPT_SHELL
      int result;

      proc->p_exited = false;

      proc->p_orphan = false;

      result = proc_children_create(proc);
      if(result) {
        kfree(proc->p_name);
        kfree(proc);
        return result;
      }

      if (strcmp(name, "[kernel]") == 0) {
        proc->p_parent_pid = 0;
        proc->p_pid = 1;
        spinlock_acquire(&pt->lock);
        pt->procs[proc->p_pid] = proc;
        spinlock_release(&pt->lock);
      } else {
        proc->p_parent_pid = curproc->p_pid;
        result = proc_table_add(proc);
        if (result) {
          proc_children_destroy(proc);
          kfree(proc->p_name);
          kfree(proc);
          return result;
        }
      }

      result = proc_fileTable_create(proc);
      if (result) {
        proc_table_remove(proc);
        proc_children_destroy(proc);
        kfree(proc->p_name);
        kfree(proc);
        return result;
      }

      proc->p_sem = sem_create(name, 0);
      if (proc->p_sem == NULL) {
        proc_fileTable_destroy(proc);
        proc_table_remove(proc);
        proc_children_destroy(proc);
        kfree(proc->p_name);
        kfree(proc);
        return ENOMEM;
      }
#endif

      *retproc = proc;

      return 0;
}

/*
 * Destroy a proc structure.
 */
void proc_destroy(struct proc *proc)
{
      /*
       * You probably want to destroy and null out much of the
       * process (particularly the address space) at exit time if
       * your wait/exit design calls for the process structure to
       * hang around beyond process exit. Some wait/exit designs
       * do, some don't.
       */

      KASSERT(proc != NULL);
      KASSERT(proc != kproc);

      /*
       * We don't take p_lock in here because we must have the only
       * reference to this structure. (Otherwise it would be
       * incorrect to destroy it.)
       */

#if OPT_SHELL
      sem_destroy(proc->p_sem);
      proc_fileTable_destroy(proc);
      proc_table_remove(proc);
      proc_children_destroy(proc);
#endif

      /* VFS fields */
      if (proc->p_cwd) {
        VOP_DECREF(proc->p_cwd);
        proc->p_cwd = NULL;
      }

      /* VM fields */
      if (proc->p_addrspace) {
        /*
         * If p is the current process, remove it safely from
         * p_addrspace before destroying it. This makes sure
         * we don't try to activate the address space while
         * it's being destroyed.
         *
         * Also explicitly deactivate, because setting the
         * address space to NULL won't necessarily do that.
         *
         * (When the address space is NULL, it means the
         * process is kernel-only; in that case it is normally
         * ok if the MMU and MMU- related data structures
         * still refer to the address space of the last
         * process that had one. Then you save work if that
         * process is the next one to run, which isn't
         * uncommon. However, here we're going to destroy the
         * address space, so we need to make sure that nothing
         * in the VM system still refers to it.)
         *
         * The call to as_deactivate() must come after we
         * clear the address space, or a timer interrupt might
         * reactivate the old address space again behind our
         * back.
         *
         * If p is not the current process, still remove it
         * from p_addrspace before destroying it as a
         * precaution. Note that if p is not the current
         * process, in order to be here p must either have
         * never run (e.g. cleaning up after fork failed) or
         * have finished running and exited. It is quite
         * incorrect to destroy the proc structure of some
         * random other process while it's still running...
         */
        struct addrspace *as;

        if (proc == curproc) {
          as = proc_setas(NULL);
          as_deactivate();
        }
        else {
          as = proc->p_addrspace;
          proc->p_addrspace = NULL;
        }
        as_destroy(as);
      }

      KASSERT(proc->p_numthreads == 0);
      spinlock_cleanup(&proc->p_lock);

      kfree(proc->p_name);
      kfree(proc);
}
/*
 * Create the process structure for the kernel.
 */
void proc_bootstrap(void)
{
      int result;

#if OPT_SHELL
      result = proc_table_create();
      if (result) {
        panic("proc_table_create failed: %s\n", strerror(result));
      }
#endif

      result = proc_create("[kernel]", &kproc);
      if (result) {
        panic("proc_create for kproc failed: %s\n", strerror(result));
      }
}

/*
 * Create a fresh proc for use by runprogram.
 * Return proper error code on error:
 * - ENOMEM: sufficient memory was not available
 * - ENPROC: there are already too many processes on the system
 * - EMPROC: parent process has too much children processes
 *
 * It will have no address space and will inherit the current
 * process's (that is, the kernel menu's) current directory.
 */
int proc_create_runprogram(const char *name, struct proc **retproc)
{
      struct proc *newproc;
      int result;

      result = proc_create(name, &newproc);
      if (result) {
        return result;
      }

      /* VM fields */
      newproc->p_addrspace = NULL;

      /* VFS fields */

      /*
       * Lock the current process to copy its current directory.
       * (We don't need to lock the new process, though, as we have
       * the only reference to it.)
       */
      spinlock_acquire(&curproc->p_lock);
      if (curproc->p_cwd != NULL) {
        VOP_INCREF(curproc->p_cwd);
        newproc->p_cwd = curproc->p_cwd;
      }
      spinlock_release(&curproc->p_lock);

#if OPT_SHELL
      /* Link child process to its parent, so that child terminates on parent exit */
      result = proc_children_add(curproc, newproc);
      if(result){
        return result;
      }
#endif

      *retproc = newproc;

      return 0;
}

/*
 * Add a thread to a process. Either the thread or the process might
 * or might not be current.
 *
 * Turn off interrupts on the local cpu while changing t_proc, in
 * case it's current, to protect against the as_activate call in
 * the timer interrupt context switch, and any other implicit uses
 * of "curproc".
 */
int proc_addthread(struct proc *proc, struct thread *t)
{
      int spl;

      KASSERT(t->t_proc == NULL);

      spinlock_acquire(&proc->p_lock);
      proc->p_numthreads++;
      spinlock_release(&proc->p_lock);

      spl = splhigh();
      t->t_proc = proc;
      splx(spl);

      return 0;
}

/*
 * Remove a thread from its process. Either the thread or the process
 * might or might not be current.
 *
 * Turn off interrupts on the local cpu while changing t_proc, in
 * case it's current, to protect against the as_activate call in
 * the timer interrupt context switch, and any other implicit uses
 * of "curproc".
 */
void proc_remthread(struct thread *t)
{
      struct proc *proc;
      int spl;

      proc = t->t_proc;
      KASSERT(proc != NULL);

      spinlock_acquire(&proc->p_lock);
      KASSERT(proc->p_numthreads > 0);
      proc->p_numthreads--;
      spinlock_release(&proc->p_lock);

      spl = splhigh();
      t->t_proc = NULL;
      splx(spl);
}

/*
 * Fetch the address space of (the current) process.
 *
 * Caution: address spaces aren't refcounted. If you implement
 * multithreaded processes, make sure to set up a refcount scheme or
 * some other method to make this safe. Otherwise the returned address
 * space might disappear under you.
 */
struct addrspace *proc_getas(void)
{
      struct addrspace *as;
      struct proc *proc = curproc;

      if (proc == NULL) {
        return NULL;
      }

      spinlock_acquire(&proc->p_lock);
      as = proc->p_addrspace;
      spinlock_release(&proc->p_lock);
      return as;
}

/*
 * Change the address space of (the current) process. Return the old
 * one for later restoration or disposal.
 */
struct addrspace *proc_setas(struct addrspace *newas)
{
      struct addrspace *oldas;
      struct proc *proc = curproc;

      KASSERT(proc != NULL);

      spinlock_acquire(&proc->p_lock);
      oldas = proc->p_addrspace;
      proc->p_addrspace = newas;
      spinlock_release(&proc->p_lock);
      return oldas;
}

#if OPT_SHELL
/*
 * Search for a process by pid in the Process Table.
 * Return proper error code on error:
 * - ESRCH: the pid argument named a nonexistent process
 */
int proc_table_search(pid_t pid, struct proc **retproc)
{
      struct proc *p;

      if ((pid < PID_MIN) || (pid > PID_MAX)) {
        return ESRCH;
      }

      spinlock_acquire(&pt->lock);
      p = pt->procs[PROCS_IDX(pid)];
      spinlock_release(&pt->lock);

      if (p == NULL) {
        return ESRCH;
      }

      KASSERT(p->p_pid == pid);

      *retproc = p;

      return 0;
}

/*
 * Wait for process termination, destroy the process, and return exit status.
 */
int proc_wait(struct proc *proc)
{
      int exit_status;

      KASSERT(proc != NULL);
      KASSERT(proc != kproc);

      /*
       * Wait on semaphore of the process.
       * If the process has already exited, don't waste time waiting on semaphore.
       */
      if (!proc->p_exited) {
        P(proc->p_sem);
      }

      /* Save exit status */
      exit_status = proc->p_exit_status;

      /* Destroy the process structure */
      proc_destroy(proc);

      return exit_status;
}

/*
 * Signal for process termination.
 */
void proc_signal(struct proc *proc) {
      struct proc *pchild;

      KASSERT(proc != NULL);

      /*
       * Remove process from list of child processes of the parent process.
       * If the parent process has already exited (process is orphan), skip.
       */
      if (!proc->p_orphan) {
        proc_children_remove(proc);
      }

      /* Check child processes */
      while (!list_isEmpty(proc->p_children)) {
        /* Retrieve child process */
        pchild = list_deleteHead(proc->p_children);

        /* Set child process as orphan */
        pchild->p_orphan = true;
      }

      /*
       * Signal on semaphore of the process.
       * If the parent process has already exited (process is orphan), don't waste time signaling on semaphore.
       */
      if (!proc->p_orphan) {
        V(proc->p_sem);
      }
}

/*
 * Copy file table from a process to another process.
 */
void proc_fileTable_copy(struct proc *psrc, struct proc *pdest)
{
      int fd;
      fcb file;

      pdest->ft=CA_duplicate(psrc->ft);

      for (fd=0; fd<CA_size(psrc->ft); fd++) {
        file=sys_fileTable_get(proc_fileTable_get(psrc, fd));

        if (file != NULL) {
          /* incr reference count */
          openfileIncrRefCount(file);
        }
      }
}

/*
 * Add on file table file descriptor of system file table.
 */
int proc_fileTable_add(struct proc *proc, int indTable)
{
  int result;

  spinlock_acquire(&(proc->p_lock));
  result=CA_add(proc->ft, copyInt(&indTable));
  spinlock_release(&(proc->p_lock));
  return result;
}

/*
 * Remove on file table file descriptor of system file table.
 */
int proc_fileTable_remove(struct proc *proc, int fd)
{
  int result;

  spinlock_acquire(&(proc->p_lock));
  result=CA_remove_byIndex(proc->ft, fd);
  spinlock_release(&(proc->p_lock));

  return result;
}

/*
 * Get on file table file descriptor of system file table.
 */
int proc_fileTable_get(struct proc *proc, int fd)
{
  int *getResult;
  if(proc==NULL || fd<0)
    return -1;

  if(fd==STDIN_FILENO || fd==STDOUT_FILENO || fd==STDERR_FILENO)
    fd++;

  getResult=CA_get_byIndex(proc->ft, fd);

  return getResult==NULL? -1: *getResult;
}

/*
 * Set on file table file descriptor of system file table.
 */
int proc_fileTable_set(struct proc *proc, int indTable, int fd)
{
  int result;

  if(fd==STDIN_FILENO || fd==STDOUT_FILENO || fd==STDERR_FILENO)
    fd++;

  spinlock_acquire(&(proc->p_lock));
  result=CA_set(proc->ft, copyInt(&indTable), fd);
  spinlock_release(&(proc->p_lock));

  return result;
}
#endif
