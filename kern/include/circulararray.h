#ifndef _CIRCULARARRAY_H_
#define _CIRCULARARRAY_H_

#include "opt-shell.h"

#if OPT_SHELL

/* Definition of single item on circular array */
typedef void* CAitem;
/* Definition of key */
typedef void* CAkey;

/* Set of functions on generic item that must be passed in order to perform action */
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

/* Create a circular array, passing max dimension of array and operations. 
The array is allocate dynamically with progessive dimension concurrently with add operation. 
Returns point of new array in case of success, NULL in error*/
cirarray CA_create(int maxD, CAoperations ops);

/* Destroy operation. 
Returns 1 = error, 0 = success */
int CA_destroy(cirarray ca);

/* Add operation, reallocate array in case of space necessity
Returns position on array where element is addes, 0 in case of error */
int CA_add(cirarray ca, CAitem it);

/* Remove operation using index
Returns 1 = error, 0 = success */
int CA_remove_byIndex(cirarray ca, int pos);

/* Get item using index of array
Returs item in case of success, NULL on error */
CAitem CA_get_byIndex(cirarray ca, int pos);

/*  Get item using as input a key
Returns vect of index of ca corrisponding to the key (first position is occupied by vector dimension) on success, NULL on error
If key is not present return a vect with 1 element filled by empty dimension*/
int* CA_get(cirarray ca, CAkey key);

/* Removes all occourence of key on ca 
Returns 1 = error/notPresent, 0 = success */
int CA_remove(cirarray ca, CAkey key);

/* Returns size = error, 0 = error */
int CA_size(cirarray ca);

/* Returns 1 = empty, 0 = error/notEmpty */
int CA_isEmpty(cirarray ca);

/* Stamp all circular array
Returns 1 = error, 0 = success */
int CA_stamp(cirarray ca);

#endif

#endif /* _CIRCULARARRAY_H_ */
