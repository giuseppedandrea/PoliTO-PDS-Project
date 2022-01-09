#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <list.h>

typedef struct node_s node_t;

struct node_s {
    void *data;
    node_t *next;
    node_t *prev;
};

struct list_s {
    node_t *head;
    node_t *tail;
    size_t cnt;
};

////////////////////////////////////////////////////////////
// internal functions

static node_t *newNode(void *data, node_t *next, node_t *prev) {
    node_t *n = kmalloc(sizeof(*n));
    if (n == NULL) {
        return NULL;
    }

    n->data = data;
    n->next = next;
    n->prev = prev;

    return n;
}

static int cmpKeys(const void *av, const void *bv, size_t len) {
    const unsigned char *a = av;
    const unsigned char *b = bv;
    size_t i;

    for (i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return (int)(a[i] - b[i]);
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////
// external interface

list list_create(void) {
    list l = kmalloc(sizeof(*l));
    if (l == NULL) {
        return NULL;
    }

    l->head = NULL;
    l->tail = NULL;
    l->cnt = 0;

    return l;
}

bool list_isEmpty(list l) {
    KASSERT(l != NULL);

    return l->cnt == 0;
}

size_t list_size(list l) {
    KASSERT(l != NULL);

    return l->cnt;
}

int list_insertHead(list l, void *data) {
    KASSERT(l != NULL);
    KASSERT(data != NULL);

    node_t *n = newNode(data, l->head, NULL);
    if (n == NULL) {
        return ENOMEM;
    }

    if (l->head != NULL) {
        l->head->prev = n;
        l->head = n;
    } else {
        l->head = l->tail = n;
    }

    l->cnt++;

    return 0;
}

int list_insertTail(list l, void *data) {
    KASSERT(l != NULL);
    KASSERT(data != NULL);

    node_t *n = newNode(data, NULL, l->tail);
    if (n == NULL) {
        return ENOMEM;
    }

    if (l->tail != NULL) {
        l->tail->next = n;
        l->tail = n;
    } else {
        l->head = l->tail = n;
    }

    l->cnt++;

    return 0;
}

void *list_searchByKey(list l, const void *key, size_t key_offset, size_t key_size) {
    KASSERT(l != NULL);
    KASSERT(key != NULL);
    KASSERT(key_size > 0);

    node_t *n;
    for (n = l->head; n != NULL; n=n->next) {
        if (cmpKeys(key, n->data + key_offset, key_size) == 0) {
            return n->data;
        }
    }

    return NULL;
}

void *list_deleteHead(list l) {
    KASSERT(l != NULL);

    if (l->head == NULL) {
        return NULL;
    }

    node_t *n = l->head;
    void *data = n->data;
    l->head = n->next;
    if (l->head == NULL) {
        l->tail = l->head;
    } else {
        l->head->prev = NULL;
    }

    kfree(n);

    l->cnt--;

    return data;
}

void *list_deleteTail(list l) {
    KASSERT(l != NULL);

    if (l->tail == NULL) {
        return NULL;
    }

    node_t *n = l->tail;
    void *data = n->data;
    l->tail = n->prev;
    if (l->tail == NULL) {
        l->head = l->tail;
    } else {
        l->tail->next = NULL;
    }

    kfree(n);

    l->cnt--;

    return data;
}

void *list_deleteByKey(list l, const void *key, size_t key_offset, size_t key_size) {
    KASSERT(l != NULL);
    KASSERT(key != NULL);
    KASSERT(key_size > 0);

    node_t *n;
    for (n = l->head; n != NULL; n=n->next) {
       if (cmpKeys(key, n->data + key_offset, key_size) == 0) {
            void *data;

            if (n == l->head) {
                data = list_deleteHead(l);
            } else if (n == l->tail) {
                data = list_deleteTail(l);
            } else {
                n->prev->next = n->next;
                n->next->prev = n->prev;
                data = n->data;
                kfree(n);
                l->cnt--;
            }

            return data;
        }
    }

    return NULL;
}

void list_destroy(list l) {
    KASSERT(l != NULL);

    node_t *n, *tmp;
    n = l->head;
    while (n != NULL) {
        tmp = n;
        n = n->next;
        kfree(tmp);
    }

    kfree(l);

    // Although the memory pointed to by l is freed, since the pointer l is
    // passed by value (this function operates on a copy of the pointer and
    // never modifies the original), the caller will be responsible for not
    // using the pointer after calling to list_destroy.

    return;
}
