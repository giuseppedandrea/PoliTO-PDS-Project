#ifndef _CIRCULARARRAY_H_
#define _CIRCULARARRAY_H_

#include "opt-shell.h"

#if OPT_SHELL

// Definition of single item on circular array
typedef void* CAitem;
typedef void* CAkey;

typedef CAitem* (*allocateCA)(int maxD);
typedef CAitem (*newCAitem)(void);
typedef int (*cmpCAitem)(CAkey a , CAkey b);
typedef void (*freeCAitem)(CAitem a);
typedef CAitem (*copyCAitem)(CAitem source);
typedef CAkey (*getKeyCAItem)(CAitem source);
typedef void (*coutCAItem)(CAitem source);

typedef struct _CAops {
    newCAitem newItem;
    cmpCAitem cmpItem;
    freeCAitem freeItem;
    copyCAitem copyItem;
    getKeyCAItem getItemKey;
    coutCAItem coutItem;
} CAoperations;

typedef struct cirarray_s *cirarray;

cirarray CA_create(int maxD, CAoperations ops);

int CA_destroy(cirarray ca);

// returns position on vect where element is added or 0 in case of error
int CA_add(cirarray ca, CAitem it);

int CA_remove_byIndex(cirarray ca, int pos);

CAitem CA_get_byIndex(cirarray ca, int pos);

// returns vect of index of ca corrisponding to the key, first position is occupied by vector dimension
int* CA_get(cirarray ca, CAkey key);

// remove all occourence of key on ca
int CA_remove(cirarray ca, CAkey key);

int CA_size(cirarray ca);

int CA_isEmpty(cirarray ca);

int CA_stamp(cirarray ca);

#endif

#endif /* _CIRCULARARRAY_H_ */
