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
#include <spl.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <syscall.h>
#include "item.h"
#include <synch.h>
#include <limits.h>

#define MAX_PROC 100


/*
 * The process for the kernel; this holds all the kernel-only threads.
 */
struct proc *kproc;

// in limits.h il numero massimo di PID e' __PID_MAX

#if OPT_SHELL


static int proc_hundreds=1;

static struct _processTable {
  int active;           /* initial value 0 */
  struct proc **proc;   /* [0] not used. pids are >= 1 */
  int dimTable;
  int last_i;           /* index of last allocated pid */
  struct spinlock lk;	/* Lock for this table */
} processTable;

#define _PROCTABLE_proc(pid) (processTable.proc[pid])

struct _children {
  pid_t *p_ch;
  int n_ch;
  int last_ch; 
}; 

/* Creating file table of single process */
static int proc_fileTable_create(struct proc *proc)
{
  CAoperations ops;
  int *stdin, *stdout, *stderr;

  if(proc==NULL)
    return 1;
  
  ops.newItem=newInt;
  ops.cmpItem=cmpInt;
  ops.freeItem=freeInt;
  ops.copyItem=copyInt;
  ops.getItemKey=getIntKey;

  proc->ft=CA_create(OPEN_MAX, ops);
  if(proc->ft==NULL)
    return 1;
  
  stdin=newInt();
  stdout=newInt();
  stderr=newInt();

  CA_add(proc->ft, stdin);
  CA_add(proc->ft, stdout);
  CA_add(proc->ft, stderr);

  return 0;
}

/* Destroy file table of single process  */
static int proc_fileTable_destroy(struct proc *proc)
{
  return CA_destroy(proc->ft);
}

struct proc *proc_search_pid(pid_t pid) {
      struct proc *p;

      KASSERT(pid>=0 && pid<__PID_MAX); // fix this

      p = _PROCTABLE_proc(pid);
      
      KASSERT(p->p_pid==pid); // fix this
      return p;
}


// returns pid added or error code
static int processTable_add(struct proc *proc, const char *name) {
      /* search a free index in table using a circular strategy */
      int i, found=0;
      struct proc **buff;
      

      spinlock_acquire(&processTable.lk);

      i = processTable.last_i+1;
      proc->p_pid = 0;

      if (i>proc_hundreds*MAX_PROC) //e' un vettore circolare, devi verificare se ci sono posizioni
                                    // libere deallocate 
        i=1;

      while (i!=processTable.last_i) {
        if (processTable.proc[i] == NULL) {
          processTable.proc[i] = proc;
          processTable.last_i = i;
          proc->p_pid = i;
          found=1;
          break;
        }
        i++;
        if (i>processTable.dimTable)
          i=1;
      }

      if(!found && processTable.dimTable<__PID_MAX) {
        // devo allocare piu' spazio nella tabella dei processi
        buff=kmalloc((++proc_hundreds)*MAX_PROC*sizeof(struct proc *));

        for(i=0; i<processTable.dimTable+1; i++)
          buff[i]=processTable.proc[i];

        // guarda appunti per queste ultime due linee di codice, ragionamento dovrebbe essere corretto
        kfree(processTable.proc); 
        processTable.proc=buff;
        processTable.last_i=processTable.dimTable+1;
        processTable.dimTable+=proc_hundreds*MAX_PROC+1;

        proc->p_pid = processTable.last_i;
        found=1;
      }

        
      spinlock_release(&processTable.lk);
  
      if (proc->p_pid==0) {
        // se si arriva qui vuol solo dire che il numero di processi che si vuole tenere
        // in memoria e' eccessivo e supera __PID_MAX
  
        /* ------------------------------------------------------------------------ 
           AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
                              COMPLETARE QUESTA PARTE
           AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
           ----------------------------------------------------------------------------*/
          panic("too many processes. proc table is full\n");

          return -1; // fix this with right error code
      }
  
      proc->p_status = 0;
      proc->p_sem = sem_create(name, 0);
      return proc->p_pid;
}


static void processTable_remove(struct proc *proc) {

      /* remove the process from the table */
      int i;
      spinlock_acquire(&processTable.lk);
      i = proc->p_pid;
      KASSERT(i>0 && i<=MAX_PROC); // gestione seria errori
      /* ------------------------------------------------------------------------ 
         AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
                                COMPLETARE QUESTA PARTE
         AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
         ----------------------------------------------------------------------------*/

      processTable.proc[i] = NULL;
      spinlock_release(&processTable.lk);

      sem_destroy(proc->p_sem);
}



static int procChildren_create(struct proc *p)
{
      int dim=(int) __PID_CHILDREN_MAX/4+1; // first number children 25 

      if(p==NULL)
        return 1; // error on passing process

      p->ch_pid=(struct _children *) kmalloc(sizeof(*(p->ch_pid)));
      p->ch_pid->p_ch=(pid_t *) kmalloc(dim*sizeof(pid_t));
      p->ch_pid->n_ch=dim;
      p->ch_pid->last_ch=0;
      
      // father pid set as same pid
      p->fath_pid=p->p_pid; 



      return 0;
}


#endif

/*
 * Create a proc structure.
 */
static struct proc *proc_create(const char *name)
{
      struct proc *proc;
      pid_t pid;

      proc = kmalloc(sizeof(*proc));
      if (proc == NULL) {
        return NULL;
      }
      proc->p_name = kstrdup(name);
      if (proc->p_name == NULL) {
        kfree(proc);
        return NULL;
      }

      proc->p_numthreads = 0;
      spinlock_init(&proc->p_lock);

      /* VM fields */
      proc->p_addrspace = NULL;

      /* VFS fields */
      proc->p_cwd = NULL;

#if OPT_SHELL
      if(processTable.active)
        {
          // im creating a user process
          pid=(pid_t) processTable_add(proc, name);
          
          if (pid==-1) // too many processes
            return NULL;

          if(procChildren_create(proc))
            return NULL; // allocation problem          
          
          if(proc_fileTable_create(proc))
            return NULL; // allocation problem
        }
      
      if(proc_fileTable_create(proc))
        return NULL;
      

#endif

      return proc;
}

/*
 * Destroy a proc structure.
 *
 * Note: nothing currently calls this. Your wait/exit code will
 * probably want to do so.
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

      processTable_remove(proc);
      procChild_remove(proc);

      proc_fileTable_destroy(proc);

      kfree(proc->p_name);
      kfree(proc);
}
/*
 * Create the process structure for the kernel.
 */
void proc_bootstrap(void)
{

      kproc = proc_create("[kernel]");
      if (kproc == NULL) {
      panic("proc_create for kproc failed\n");
      }

#if OPT_SHELL
      spinlock_init(&processTable.lk);
      /* kernel process is not registered in the table */
      processTable.dimTable=proc_hundreds*100+1;
      processTable.proc=kmalloc(processTable.dimTable*sizeof(struct proc *));
      

      processTable.active = 1;

#endif


}


/*
 * Create a fresh proc for use by runprogram.
 *
 * It will have no address space and will inherit the current
 * process's (that is, the kernel menu's) current directory.
 */
struct proc *proc_create_runprogram(const char *name)
{
      struct proc *newproc;

      newproc = proc_create(name);
      if (newproc == NULL) {
        return NULL;
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

      return newproc;
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
int proc_wait(struct proc *proc)
{
      int return_status;
      /* NULL and kernel proc forbidden */
      KASSERT(proc != NULL); // gestire questo con errore?
      /* ------------------------------------------------------------------------ 
         AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
                              COMPLETARE QUESTA PARTE
         AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
         ----------------------------------------------------------------------------*/

      KASSERT(proc != kproc);

      /* wait on semaphore*/ 
      proc_signal_wait(proc);

      return_status = proc->p_status;
      proc_destroy(proc);
      return return_status;
}


#endif

#if OPT_FILE
void proc_file_table_copy(struct proc *psrc, struct proc *pdest) {
      int fd;
      fcb file;

      pdest->ft=CA_duplicate(psrc->ft);
      for (fd=0; fd<CA_size(psrc->ft); fd++) {
        file=sys_fileTable_get(proc_fileTable_get(psrc, fd));

        if (file != NULL) {
          /* incr reference count */
          openfileIncrRefCount(file);
          fd--;
        }
      }
}
#endif

#if OPT_SHELL

int procChild_add(struct proc *fath, struct proc *ch)
{
      pid_t fath_pid, ch_pid;
      int i, found=0;
      

      if(fath==NULL || ch==NULL) // process passed are not allocated
        return 1;

      KASSERT(fath!=kproc); // father process can't be kproc
      
      fath_pid=fath->p_pid;
      ch_pid=ch->p_pid;
      
      ch->fath_pid=fath_pid;

      i = fath->ch_pid->last_ch+1;

      if(i>fath->ch_pid->n_ch)
        i=1;

      while(i != fath->ch_pid->last_ch)
        {
          if(fath->ch_pid->p_ch[i]==0){
            fath->ch_pid->p_ch[i]=ch_pid;
            fath->ch_pid->last_ch=i;
            found=1;
            break;
          }

          i++;
          
          if(i > fath->ch_pid->n_ch)
            i=1;

        }

      if(!found)
      {
          if(fath->ch_pid->n_ch<__PID_CHILDREN_MAX)
          {
            // must implement realloc of struct pid_t *p_ch

          }
        else
          {
            // too much children process, error.
            // DA RIVEDERE
            panic("too many children processes.\n");
            return 1;

          }
      }
      return 0;
}

int procChild_remove(struct proc *proc)
{

      KASSERT(proc!=NULL); //da rivedere

      // remove process from the father list
      if(proc->fath_pid!=proc->p_pid)
      {
        // Gestire tutti errori
        _PROCTABLE_proc(proc->fath_pid)->ch_pid->p_ch[proc->p_pid]=0;
      }
      // gestire deallocazione proc->children

    return 0; //gestire errori
}

void proc_signal_wait(struct proc *proc)
{
      P(proc->p_sem);
}


void proc_signal_end(struct proc *proc)
{
      V(proc->p_sem);
}



/* Adding on file table file descriptor of system file table */
int proc_fileTable_add(struct proc *proc, int indTable)
{
  return CA_add(proc->ft, copyInt(&indTable));
}

/* Remove on file table file descriptor of system file table*/
int proc_fileTable_remove(struct proc *proc, int fd)
{
  return CA_remove_byIndex(proc->ft, fd);
}

int proc_fileTable_get(struct proc *proc, int fd)
{
  int *getResult;
  if(proc==NULL || fd<0)
    return -1;

  getResult=CA_get_byIndex(proc->ft, fd);

  return getResult==NULL? -1: *getResult;
}

#endif
