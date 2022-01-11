#include <types.h>
#include <lib.h>
#include "circulararray.h"
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
CAkey _getItemKey(CAitem source);
void _coutItem(CAitem source);

CAitem _newCAitem(void)
{
    item new=kmalloc(sizeof(*new));
    bzero(new, sizeof(*new));
    new->data=++count;
    new->name=kmalloc(MAX_STRING*sizeof(char));
    bzero(new->name, MAX_STRING*sizeof(char));

    return new;
}

 int _cmpCAitem(CAkey a , CAkey b)
 {
     int *item_a, *item_b;

     item_a=(int *) a;
     item_b=(int *) b;

     return (*item_a) - (*item_b);
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

CAkey _getItemKey(CAitem source)
{
    item itsource=(item) source;

    return &(itsource->data);
}

void _coutItem(CAitem source) {
    item it=(item) source;
    kprintf("%d \n", it->data);
}

static void carraytest_a(CAoperations ops) {
    cirarray ca;
    count=0;

    kprintf("Test a doing...\n");
    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    CA_destroy(ca);
    KASSERT(ca!=NULL);

    kprintf("Test a done.\n");
}

static void carraytest_b(CAoperations ops) {
    int i, result, n;
    cirarray ca;
    count=0;

    kprintf("Test b doing...\n");
    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    n=30;
    for(i=0; i<n; i++)
        result=CA_add(ca, ops.newItem());	
    
    KASSERT(result != 0);
    KASSERT(CA_size(ca) == n);    

    CA_destroy(ca);
    kprintf("Test b done.\n");
}

static void carraytest_c(CAoperations ops) {
    int i, result, n;
    int *vect;
    cirarray ca;
    count=0;

    kprintf("Test c doing...\n");
    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    n=30;
    vect=kmalloc(n*sizeof(int));
    for(i=0; i<n; i++) {
        result=CA_add(ca, ops.newItem());	
        if(result!=0)
            vect[i]=result;
    }
    KASSERT(result != 0);
    KASSERT(CA_size(ca) == n);

    for(i=0; i<n; i++) {
        result=CA_remove_byIndex(ca, vect[i]);	
        KASSERT(result == 0);
    }

    KASSERT(CA_isEmpty(ca));

    CA_destroy(ca);
    kprintf("Test c done.\n");
}

static void carraytest_d(CAoperations ops) {
    int i, result, n;
    int *vect;
    cirarray ca;
    item it;
    count=0;

    kprintf("Test d doing...\n");

    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    n=30;
    vect=kmalloc(n*sizeof(int));
    for(i=0; i<n; i++) {
        result=CA_add(ca, ops.newItem());	
        if(result!=0)
            vect[i]=result;
    }
    KASSERT(result != 0);
    KASSERT(CA_size(ca) == n);

    result=0;

    for(i=0; i<n; i++) {
        it=CA_get_byIndex(ca, vect[i]);
        if(it==NULL) {
            result=1;
            break;
        }
        _coutItem(it);
    }

    KASSERT(result == 0);
    
    for(i=0; i<n; i++) {
        result=CA_remove_byIndex(ca, vect[i]);	
        KASSERT(result == 0);
    }

    KASSERT(CA_isEmpty(ca));

    CA_destroy(ca);
    kprintf("Test d done.\n");
}

static void carraytest_e(CAoperations ops) {
    int i, result, n;
    int *vect, key;
    cirarray ca;
    item it;
    count=0;

    kprintf("Test e doing...\n");

    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    n=29;
    for(i=0; i<n; i++) {
        result=CA_add(ca, ops.newItem());	
        KASSERT(result != 0);

    }
    KASSERT(CA_size(ca) == n);


    kprintf("Adding items...\n");
    result=CA_add(ca, ops.copyItem(CA_get_byIndex(ca, 2)));
    result=CA_add(ca, ops.copyItem(CA_get_byIndex(ca, 4)));
    result=CA_add(ca, ops.copyItem(CA_get_byIndex(ca, 4)));
    result=CA_add(ca, ops.copyItem(CA_get_byIndex(ca, 4)));
    result=CA_add(ca, ops.copyItem(CA_get_byIndex(ca, 7)));
    result=CA_add(ca, ops.copyItem(CA_get_byIndex(ca, 20)));

    key=2;
    vect=CA_get(ca, &key);
    KASSERT(vect!=NULL);

    kprintf("Getting items with 2 value...\n");
    for(i=0; i<vect[0]; i++) {
        it=CA_get_byIndex(ca, vect[i+1]);	
        KASSERT(it != NULL);
        _coutItem(it);
    }

    kprintf("Stamp CA.\n");
    CA_stamp(ca);

    kprintf("Removing items with 2 and 4 value...\n");
    key=2;
    result=CA_remove(ca, &key);
    key=4;
    result=CA_remove(ca, &key);
    KASSERT(result==0);

    kprintf("Stamp CA.\n");
    CA_stamp(ca);

    CA_destroy(ca);
    kprintf("Test e done.\n");
}

static void carraytest_f(CAoperations ops) {
    int i, result, n;
    cirarray ca, cadst;
    count=0;

    kprintf("Test f doing...\n");

    ca=CA_create(100, ops);
    KASSERT(CA_isEmpty(ca));

    n=29;
    for(i=0; i<n; i++) {
        result=CA_add(ca, ops.newItem());	
        KASSERT(result != 0);

    }
    KASSERT(CA_size(ca) == n);

    kprintf("Stamp CA.\n");
    CA_stamp(ca);

    kprintf("Duplicating CA...\n");

    cadst=CA_duplicate(ca);
    kprintf("CA duplicated.\n");

    kprintf("Stamp CA duplicated...\n");
    CA_stamp(cadst);
    
    kprintf("Well done.\n");
    
    CA_destroy(ca);
    CA_destroy(cadst);
    kprintf("Test f done.\n");
}

int carraytest(int nargs, char **args) {
    //cirarray ca;
    CAoperations ops;

    (void)nargs;
    (void)args;

    ops.newItem=_newCAitem;
    ops.cmpItem=_cmpCAitem;
    ops.freeItem=_freeCAitem;
    ops.copyItem=_copyCAitem;
    ops.getItemKey=_getItemKey;
    ops.coutItem=_coutItem;

    kprintf("Testing circular array...\n");

    carraytest_a(ops);
    carraytest_b(ops);
    carraytest_c(ops);
    carraytest_d(ops);
    carraytest_e(ops);
    carraytest_f(ops);


    kprintf("Done.\n");

    return 0;
}

