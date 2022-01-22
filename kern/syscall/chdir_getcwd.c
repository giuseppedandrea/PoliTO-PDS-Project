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

int sys_chdir(const char *pathname, int *errp)
{
    int result;
    char *path;

    if(pathname==NULL) {
        *errp=EFAULT;
        return -1;
    }

    path=kmalloc((strlen(pathname)*sizeof(char)));
    strcpy(path, pathname);

    result=vfs_chdir(path);
    if(result) {
        *errp=result;
        return -1;
    }

    return 0;
}

