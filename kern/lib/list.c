#include <types.h>
#include <lib.h>
#include <list.h>

typedef struct node_s node_t;

struct node_s {
    void *data;
    node_t *next;
    node_t *prev;
};

struct list_s {
    void *data;
    node_t *head;
    node_t *tail;
};


node_t *newNode(const void *data, node_t *next, node_t *prev) {
    node_t *n = kmalloc(sizeof(*n));
    if (n == NULL) {
        return NULL;
    }

    n->data = data;
    n->next = next;
    n->prev = prev;

    return n;
}

int cmpKeys(const void *av, const void *bv, size_t len) {
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

list_t *list_create(void) {
    list_t *l = kmalloc(sizeof(*l));
    if (l == NULL) {
        return NULL;
    }

    l->head = NULL;
    l->tail = NULL;

    return l;
}

bool list_insertHead(list_t *l, void *data) {
    if ((l == NULL) || (data == NULL)) {
        return false;
    }

    node_t *n = newNode(data, l->head, NULL);
    if (n == NULL) {
        return false;
    }

    if (l->head != NULL) {
        l->head->prev = n;
        l->head = n;
    } else {
        l->head = l->tail = n;
    }

    return true;
}

bool list_insertTail(list_t *l, void *data) {
    if ((l == NULL) || (data == NULL)) {
        return false;
    }

    node_t *n = newNode(data, NULL, l->tail);
    if (n == NULL) {
        return false;
    }

    if (l->tail != NULL) {
        l->tail->next = n;
        l->tail = n;
    } else {
        l->head = l->tail = n;
    }

    return true;
}

void *list_searchByKey(list_t *l, void *key, size_t key_offset, size_t key_size) {
    if (l == NULL) {
        return NULL;
    }

    node_t *n;
    for (n = l->head; n != NULL; n=n->next) {
        if (cmpKeys(key, n->data + key_offset, key_size) == 0) {
            return n->data;
        }
    }

    return NULL;
}

bool list_deleteHead(list_t *l) {
    if ((l == NULL) || (l->head == NULL)) {
        return false;
    }

    node_t *n = l->head;
    l->head = n->next;
    if (l->head == NULL) {
        l->tail = l->head;
    } else {
        l->head->prev = NULL;
    }

    free(n);

    return true;
}

bool list_deleteTail(list_t *l) {
    if ((l == NULL) || (l->tail == NULL)) {
        return false;
    }

    node_t *n = l->tail;
    l->tail = n->prev;
    if (l->tail == NULL) {
        l->head = l->tail;
    } else {
        l->tail->next = NULL;
    }

    free(n);

    return true;
}

bool list_deleteByKey(list_t *l, void *key, size_t key_offset, size_t key_size) {
    if (l == NULL) {
        return false;
    }

    node_t *n;
    for (n = l->head; n != NULL; n=n->next) {
        if (cmpKeys(key, n->data + key_offset, key_size) == 0) {
            if (n == l->head) {
                list_deleteHead(l);
            } else if (n == l->tail) {
                list_deleteTail(l);
            } else {
                n->prev->next = n->next;
                n->next->prev = n->prev;
                free(n);
            }

            return true;
        }
    }

    return false;
}

void *list_extractHead(list_t *l) {
    if (l == NULL) {
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

    free(n);

    return data;
}

void *list_extractTail(list_t *l) {
    if (l == NULL) {
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

    free(n);

    return data;
}

void *list_extractByKey(list_t *l, void *key, size_t key_offset, size_t key_size) {
    if (l == NULL) {
        return NULL;
    }

    node_t *n;
    void *data;
    for (n = l->head; n != NULL; n=n->next) {
       if (cmpKeys(key, n->data + key_offset, key_size) == 0) {
            if (n == l->head) {
                list_extractHead(l);
            } else if (n == l->tail) {
                list_extractTail(l);
            } else {
                n->prev->next = n->next;
                n->next->prev = n->prev;
                data = n->data;
                free(n);
            }

            return data;
        }
    }

    return NULL;
}

void list_destroy(list_t *l) {
    if (l == NULL) {
        return;
    }

    node_t *n;
    for (n = l->head; n != NULL; n = n->next) {
        kfree(n);
    }
    kfree(l);

    return;
}
