#ifndef _LIST_H_
#define _LIST_H_

/*
 * Unordered Linked List ADT
 *
 * Functions:
 *  list_create      - create list.
 *  list_insertHead  - insert a node in head.
 *  list_insertTail  - insert a node in tail.
 *  list_search      - search a node by its key
 *  list_deleteHead  - delete a node in head.
 *  list_deleteTail  - delete a node in tail.
 *  list_deleteKey   - delete a node by its key.
 *  list_extractHead - extract a node in head.
 *  list_extractTail - extract a node in tail.
 *  list_extractKey  - extract a node by its key.
 *  list_destroy     - destroy list.
 */


typedef struct list_s list_t;  /* Opaque */

list_t *list_create(void);
bool    list_insertHead(list_t *, void *data);
bool    list_insertTail(list_t *, void *data);
void   *list_searchByKey(list_t *, void *key, size_t key_offset, size_t key_size);
bool    list_deleteHead(list_t *);
bool    list_deleteTail(list_t *);
bool    list_deleteByKey(list_t *, void *key, size_t key_offset, size_t key_size);
void   *list_extractHead(list_t *);
void   *list_extractTail(list_t *);
void   *list_extractByKey(list_t *, void *key, size_t key_offset, size_t key_size);
void    list_destroy(list_t *);


#endif /* _LIST_H_ */
