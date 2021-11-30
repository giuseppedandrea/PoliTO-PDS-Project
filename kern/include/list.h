#ifndef _LIST_H_
#define _LIST_H_

#include "opt-shell.h"

#if OPT_SHELL
/*
 * Unordered Linked List ADT
 */

typedef struct list_s *list;    /* Opaque pointer */

/* create list */
list    list_create(void);
/* check if the list is empty */
bool    list_isEmpty(list l);
/* return the current number of nodes in the list */
size_t  list_size(list l);
/* insert a node in head */
bool    list_insertHead(list l, void *data);
/* insert a node in tail */
bool    list_insertTail(list l, void *data);
/* search a node by its key and, if found, return the associated data */
void   *list_searchByKey(list l, const void *key, size_t key_offset, size_t key_size);
/* delete a node in head and return the associated data */
void   *list_deleteHead(list l);
/* delete a node in tail and return the associated data */
void   *list_deleteTail(list l);
/* delete a node by its key and return the associated data */
void   *list_deleteByKey(list l, const void *key, size_t key_offset, size_t key_size);
/* destroy list */
void    list_destroy(list l);
#endif

#endif /* _LIST_H_ */
