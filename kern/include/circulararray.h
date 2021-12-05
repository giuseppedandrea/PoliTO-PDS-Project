#ifndef _CIRCULARARRAY_H_
#define _CIRCULARARRAY_H_

#include "opt-shell.h"

#if OPT_SHELL

//#include "item.h"

// Definition of single item on circular array
typedef void* CAitem;

typedef CAitem* (*allocateCA)(int maxD);
typedef CAitem (*newCAitem)(void);
typedef int (*cmpCAitem)(CAitem a , CAitem b);
typedef void (*freeCAitem)(CAitem a);
typedef CAitem (*copyCAitem)(CAitem source);

typedef struct _CAops {
    allocateCA allocItems;
    newCAitem newItem;
    cmpCAitem cmpItem;
    freeCAitem freeItem;
    copyCAitem copyItem;
} CAoperations;

typedef struct cirarray_s *cirarray;


cirarray CA_create(int maxD, CAoperations ops);

int CA_destroy(cirarray ca);

int CA_add(cirarray ca, CAitem it);


int CA_size(cirarray ca);

int CA_isEmpty(cirarray ca);








/* cirarray CA_create(int maxD, int nf);

//int CA_destroy(cirarray ca);

//int CA_add(cirarray ca, item item);

int CA_isEmpty(cirarray ca);

 */




#endif

#endif /* _CIRCULARARRAY_H_ */
