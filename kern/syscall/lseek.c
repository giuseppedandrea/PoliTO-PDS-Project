#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <clock.h>
#include <syscall.h>
#include <current.h>
#include <lib.h>
#include <copyinout.h>
#include <vnode.h>
#include <vfs.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>
#include "item.h"
#include <kern/seek.h>


off_t sys_lseek(int fd, off_t offset, int whence, int *errp)
{
    fcb file;
    off_t calcOff;

    if(fd<0 || fd>OPEN_MAX) {
        *errp=EBADF;
        return -1;
    }

    file=sys_fileTable_get(proc_fileTable_get(curproc, fd));
    if(file==NULL) {
        *errp=EBADF;
        return -1;
    }

    if(!VOP_ISSEEKABLE(file->vn)) {
        *errp=ESPIPE;
        return -1;
    }

    lock_acquire(file->vn_lk);

    switch (whence)
    {
        case SEEK_SET:
                calcOff=offset;                
            break;
        case SEEK_CUR:
                calcOff=file->offset+offset;
            break;
        case SEEK_END:
                calcOff=file->size+offset;
            break;
        default: {
            *errp=EINVAL;
            lock_release(file->vn_lk);
            return -1;
        }
        break;
    }

    if(calcOff<0) {
        *errp=EINVAL;
        lock_release(file->vn_lk);
        return -1;
    }

    file->offset=calcOff;
    lock_release(file->vn_lk);

    return (off_t) calcOff; 
}