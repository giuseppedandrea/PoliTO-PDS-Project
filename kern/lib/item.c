#include "item.h"

CAitem newFCB(void)
{
    fcb new=kmalloc(sizeof(*new));
    bzero(new, sizeof(*new));
    return new==NULL? NULL:new;
}

CAitem newFCB_filled(struct vnode *v, off_t offset, unsigned int countRef, struct lock *vn_lk)
{
    struct stat *st=kmalloc(sizeof(struct stat));
    fcb new=kmalloc(sizeof(*new));
    bzero(new, sizeof(*new));

    if(new==NULL)
        return NULL;


    VOP_STAT(v, st);
    new->vn=v;
    new->offset=offset;
    new->vn_lk=vn_lk;
    new->countRef=countRef;
    new->size=st->st_size;

    return new;
}

int cmpFCB(CAkey a , CAkey b)
{
    struct vnode *vn1, *vn2;

    vn1=(struct vnode *) a;
    vn2=(struct vnode *) b;
    return vn1==vn2;
}

void freeFCB(CAitem a)
{
    fcb source=(fcb) a;

    vfs_close(source->vn);
    lock_destroy((source->vn_lk));
    kfree(a);
}

CAitem copyFCB(CAitem source)
{
    fcb a=(fcb) source;
    fcb b=newFCB();

    b->vn=a->vn;
    b->vn_lk=a->vn_lk;
    b->offset=a->offset;
    b->countRef=a->countRef;
    b->size=a->size;

    return b;
}

CAkey getFCBKey(CAitem source)
{
    fcb a=(fcb) source;

    return a->vn;
}

CAitem newInt(void)
{
    int *new=kmalloc(sizeof(int));
    bzero(new, sizeof(int));

    return new;
}

int cmpInt(CAkey a , CAkey b)
{
    return *((int *) a) - *((int *) b);
}

void freeInt(CAitem a)
{
    int *source=(int *) a;
    kfree(source);
}

CAitem copyInt(CAitem source)
{
    int *dest=newInt();
    int *src=(int *) source;
    *dest=*src;
    return dest;
}

CAkey getIntKey(CAitem source)
{
    return source;
}