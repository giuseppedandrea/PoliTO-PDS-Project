#include <types.h>
#include <lib.h>
#include "circulararray.h"
//#include "item.h"
#include <test.h>

#define MAX_STRING 10

static int count=0;

typedef struct item_t *item;

struct item_t
{
    int data;
    char *name;
};

CAitem* allocateCAitems(int maxD);
CAitem _newCAitem(void);
int _cmpCAitem(CAitem a , CAitem b);
void _freeCAitem(CAitem a);
CAitem _copyCAitem(CAitem source);


CAitem* allocateCAitems(int maxD)
{
    item *vect;

    vect=kmalloc(maxD*sizeof(item));

    return (CAitem*) vect;
}

CAitem _newCAitem(void)
{
    item new=kmalloc(sizeof(*new));
    bzero(new, sizeof(*new));
    new->data=++count;
    new->name=kmalloc(MAX_STRING*sizeof(char));
    bzero(new->name, MAX_STRING*sizeof(char));

    return new;
}

 int _cmpCAitem(CAitem a , CAitem b)
 {
     item item_a, item_b;

     item_a=(item) a;
     item_b=(item) b;

     return item_a->data<item_b->data;
 }

void _freeCAitem(CAitem a)
{
    item b;
    if(a==NULL)
        return;

    b=(item) a;
    kfree(b->name);
    kfree(b);
    return;
}

CAitem _copyCAitem(CAitem source)
{
    item source_it=(item) source;
    item new=kmalloc(sizeof(*new));

    new->data=source_it->data;
    new->name=kmalloc(MAX_STRING*sizeof(char));
    new->name=strcpy(new->name, source_it->name);

    return new;
}


static void carraytest_a(CAoperations ops) {
    cirarray ca;

    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    CA_destroy(ca);
    KASSERT(ca!=NULL);
}

static void carraytest_b(CAoperations ops) {
    int i, result, n;
    cirarray ca;

    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    n=30;
    for(i=0; i<n; i++)
        result=CA_add(ca, ops.newItem());	
    
    KASSERT(result == 0);
    KASSERT(CA_size(ca) == n);    

    CA_destroy(ca);
}


int carraytest(int nargs, char **args) {
    //cirarray ca;
    CAoperations ops;

    (void)nargs;
    (void)args;

    ops.allocItems=allocateCAitems;
    ops.newItem=_newCAitem;
    ops.cmpItem=_cmpCAitem;
    ops.freeItem=_freeCAitem;
    ops.copyItem=_copyCAitem;

    kprintf("Testing circular array...\n");

    carraytest_a(ops);
    carraytest_b(ops);


    kprintf("Done.\n");

    return 0;
}


