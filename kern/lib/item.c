#include "item.h"

CAitem newFCB(void)
{
    fcb new=kmalloc(sizeof(*new));
    bzero(new, sizeof(*new));
    return new==NULL? NULL:new;
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

    vnode_cleanup(source->vn);
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