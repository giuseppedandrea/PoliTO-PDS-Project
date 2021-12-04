#include <types.h>
#include <lib.h>
#include <list.h>
#include <test.h>

#define NUM 7
static const size_t keys[NUM] = {5, 32, 6, 8, 19, 1, 7};

struct data {
    char *name;
    size_t key;
    void *ptr;
};

static struct data *fakedata[NUM];

////////////////////////////////////////////////////////////
// fakedata

/*
 * Create a dummy struct data that we can put on lists for testing.
 */
static struct data *fakedata_create(size_t key) {
    struct data *d;

    d = kmalloc(sizeof(*d));
    if (d == NULL) {
        panic("listtest: Out of memory\n");
    }

    /* ignore most of the fields, zero everything for tidiness */
    bzero(d, sizeof(*d));
    /* assing the key */
    d->key = key;

    return d;
}

/*
 * Destroy a fake data.
 */
static void fakedata_destroy(struct data *d) {
    kfree(d);
}

////////////////////////////////////////////////////////////
// tests

static void listtest_a(void) {
    list l;
    size_t size;

    l = list_create();
    KASSERT(list_isEmpty(l));
    size = list_size(l);
    KASSERT(size == 0);

    list_destroy(l);
}

static void listtest_b(void) {
    list l;
    struct data *d;
    size_t size;

    l = list_create();

    list_insertHead(l, fakedata[0]);
    size = list_size(l);
    KASSERT(size == 1);
    d = list_deleteHead(l);
    KASSERT(d == fakedata[0]);
    size = list_size(l);
    KASSERT(size == 0);

    list_insertTail(l, fakedata[0]);
    size = list_size(l);
    KASSERT(size == 1);
    d = list_deleteTail(l);
    KASSERT(d == fakedata[0]);
    size = list_size(l);
    KASSERT(size == 0);

    list_destroy(l);
}

static void listtest_c(void) {
    list l;
    struct data *d;
    size_t size;

    l = list_create();

    list_insertHead(l, fakedata[0]);
    list_insertHead(l, fakedata[1]);
    size = list_size(l);
    KASSERT(size == 2);

    d = list_deleteHead(l);
    KASSERT(d == fakedata[1]);
    d = list_deleteHead(l);
    KASSERT(d == fakedata[0]);
    size = list_size(l);
    KASSERT(size == 0);

    list_insertTail(l, fakedata[0]);
    list_insertTail(l, fakedata[1]);
    size = list_size(l);
    KASSERT(size == 2);

    d = list_deleteTail(l);
    KASSERT(d == fakedata[1]);
    d = list_deleteTail(l);
    KASSERT(d == fakedata[0]);
    size = list_size(l);
    KASSERT(size == 0);

    list_destroy(l);
}

static void listtest_d(void) {
    list l;
    struct data *d;
    size_t size;

    l = list_create();

    list_insertHead(l, fakedata[0]);
    list_insertTail(l, fakedata[1]);
    size = list_size(l);
    KASSERT(size == 2);

    d = list_deleteHead(l);
    KASSERT(d == fakedata[0]);
    d = list_deleteTail(l);
    KASSERT(d == fakedata[1]);
    size = list_size(l);
    KASSERT(size == 0);


    list_insertHead(l, fakedata[0]);
    list_insertTail(l, fakedata[1]);
    size = list_size(l);
    KASSERT(size == 2);

    d = list_deleteTail(l);
    KASSERT(d == fakedata[1]);
    d = list_deleteHead(l);
    KASSERT(d == fakedata[0]);
    size = list_size(l);
    KASSERT(size == 0);

    list_destroy(l);
}

static void listtest_e(void) {
    list l;
    struct data *d;
    size_t size, key_offset, key_size;

    l = list_create();

    list_insertHead(l, fakedata[0]);
    list_insertHead(l, fakedata[1]);
    size = list_size(l);
    KASSERT(size == 2);

    key_offset = (unsigned char *)(&(d->key)) - (unsigned char *)d;
    key_size = sizeof(keys[0]);

    d = list_deleteByKey(l, &keys[0], key_offset, key_size);
    KASSERT(d == fakedata[0]);
    d = list_deleteByKey(l, &keys[1], key_offset, key_size);
    KASSERT(d == fakedata[1]);
    d = list_deleteByKey(l, &keys[2], key_offset, key_size);
    KASSERT(d == NULL);
    size = list_size(l);
    KASSERT(size == 0);

    list_destroy(l);
}

static void listtest_f(void) {
    list l;
    struct data *d;
    size_t size, key_offset, key_size;

    l = list_create();

    list_insertHead(l, fakedata[0]);
    list_insertHead(l, fakedata[1]);
    size = list_size(l);
    KASSERT(size == 2);

    key_offset = (unsigned char *)(&(d->key)) - (unsigned char *)d;
    key_size = sizeof(keys[0]);

    d = list_searchByKey(l, &keys[0], key_offset, key_size);
    KASSERT(d == fakedata[0]);
    d = list_searchByKey(l, &keys[1], key_offset, key_size);
    KASSERT(d == fakedata[1]);
    d = list_searchByKey(l, &keys[2], key_offset, key_size);
    KASSERT(d == NULL);
    size = list_size(l);
    KASSERT(size == 2);

    list_destroy(l);
}

////////////////////////////////////////////////////////////
// external interface

int listtest(int nargs, char **args) {
    size_t i;

    (void)nargs;
    (void)args;

    kprintf("Testing lists...\n");

    for (i = 0; i < NUM; i++) {
        fakedata[i] = fakedata_create(keys[i]);
    }

    listtest_a();
    listtest_b();
    listtest_c();
    listtest_d();
    listtest_e();
    listtest_f();

    for (i = 0; i < NUM; i++) {
        fakedata_destroy(fakedata[i]);
        fakedata[i] = NULL;
    }

    kprintf("Done.\n");

    return 0;
}
