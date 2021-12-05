
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include "itemCA.h"

/* static int pos=0;



struct item_s_1 {
    int data;
    char *name;
};

item_1 ITEM_new_1(void)
{
    item_1 new=kmalloc(sizeof(*new));

    new->name=kmalloc(10*sizeof(char));
    new->data=++pos;

    return new;
}

int ITEM_new(items it)
{
    switch (it->nf)
    {
    case 1:
        {
            it->newit=ITEM_new_1();
            return 0;
        }
        break;

    default: return 1;
        break;
    }

    return 1;
}
/*
int ITEM_delete(item_1 it)
{
    if(it==NULL)
        return -1;

    kfree(it->name);
    kfree(it);
    
    return 0;
}*/ 