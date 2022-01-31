/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *  The President and Fellows of Harvard College.
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
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than runprogram() does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <copyinout.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
#if OPT_SHELL
int
runprogram(char *progname, int argc, char *argv[])
{
#else
int
runprogram(char *progname)
{
#endif
  struct addrspace *as;
  struct vnode *v;
  vaddr_t entrypoint, stackptr;
  int result;

  /* Open the file. */
  result = vfs_open(progname, O_RDONLY, 0, &v);
  if (result) {
    return result;
  }

  /* We should be a new process. */
  KASSERT(proc_getas() == NULL);

  /* Create a new address space. */
  as = as_create();
  if (as == NULL) {
    vfs_close(v);
    return ENOMEM;
  }

  /* Switch to it and activate it. */
  proc_setas(as);
  as_activate();

  /* Load the executable. */
  result = load_elf(v, &entrypoint);
  if (result) {
    /* p_addrspace will go away when curproc is destroyed */
    vfs_close(v);
    return result;
  }

  /* Done with the file now. */
  vfs_close(v);

  /* Define the user stack in the address space */
  result = as_define_stack(as, &stackptr);
  if (result) {
    /* p_addrspace will go away when curproc is destroyed */
    return result;
  }

#if OPT_SHELL
  int i;
  vaddr_t *argv_ptrs;
  size_t len, argv_ptrs_size;

  KASSERT(argc >= 1);

  /* Allocate in the kernel address space a temporary array to store
     argv poiters */
  argv_ptrs_size = (argc + 1) * sizeof(vaddr_t);
  argv_ptrs = kmalloc(argv_ptrs_size);
  if (argv_ptrs == NULL) {
    return ENOMEM;
  }

  /* Load the argv strings onto the stack of the user address space */
  argv_ptrs[argc] = 0;
  for (i = argc - 1; i >= 0; i--) {
    len = strlen(argv[i]) + 1;
    stackptr -= len;
    argv_ptrs[i] = stackptr;
    result = copyout(argv[i], (userptr_t)stackptr, len);
    if (result) {
      kfree(argv_ptrs);
      return result;
    }
  }

  /* Adjust the stack pointer to be an address multiple of 8 because
     the largest representable data (double) is 8 bytes */
  stackptr -= argv_ptrs_size;
  if (stackptr % 8) {
    stackptr -= stackptr % 8;
  }

  KASSERT((stackptr % 8) == 0);

  /* Load the argv pointers onto the stack of the user address space */
  result = copyout(argv_ptrs, (userptr_t)stackptr, argv_ptrs_size);
  if (result) {
    kfree(argv_ptrs);
    return result;
  }

  /* Free the memory allocated in the kernel space for the temporary
     array of argv pointers */
  kfree(argv_ptrs);

  /* Stack layout of the user address space
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
  * +------------------+ */

  /* Warp to user mode. */
  enter_new_process(argc /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
    NULL /*userspace addr of environment*/,
    stackptr, entrypoint);
#else
  /* Warp to user mode. */
  enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
    NULL /*userspace addr of environment*/,
    stackptr, entrypoint);
#endif

  /* enter_new_process does not return. */
  panic("enter_new_process returned\n");
  return EINVAL;
}
