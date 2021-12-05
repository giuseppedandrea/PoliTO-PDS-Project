#include <types.h>
#include <lib.h>
#include "circulararray.h"


struct cirarray_s{
    CAitem *item;
    int nelements;
    int lastpos;
    int curdim;
    int maxdim;
    CAoperations ops;
};

cirarray CA_create(int maxD, CAoperations ops)
{
    cirarray ca;

    if(!(maxD>0))
        return NULL;

    ca=kmalloc(sizeof(*ca));
    ca->maxdim=maxD;
    ca->curdim=25; // dummy, if the dimention is less than 25 can be a problem. Return on ending to correct this
    ca->lastpos=0; // position on 0 is always empty 
    ca->nelements=0;
    ca->ops=ops;
    ca->item=ops.allocItems(ca->curdim);

    return ca;
}

int CA_destroy(cirarray ca)
{
    int i;

    if(ca==NULL)
        return 1; //passing error
    if(ca->nelements==0)
        {
            //CA is empty
            kfree(ca);
            return 0; 
        }

    for(i=1; i<ca->curdim; i++)
        if(ca->item[i]!=NULL)
            ca->ops.freeItem(ca->item[i]);
    

    kfree(ca);
    return 0;
}

static void CA_realloc(cirarray ca) 
{


}

int CA_add(cirarray ca, CAitem it)
{
    int i=0, found=0;

    if(ca==NULL || it==NULL)
        return 1;
    
    i=ca->lastpos+1;

    if(i>ca->curdim)
        i=1;
    
    while (i != ca->lastpos) {
        if (ca->item[i] == NULL) {
            ca->item[i]=it;
            ca->lastpos=i;
            ca->nelements++;
            found=1;
            break;
        } 

        i++;
        if(i>ca->curdim)
            i=1;
    }

    if(!found)
    {
        if(ca->curdim < ca->maxdim) {
            // it's necerrary to allocate more space
            CA_realloc(ca);
        }
        else {
            // no more space. Return error
            return 1;
        }
    }

    return 0;
}

int CA_size(cirarray ca)
{
    return ca->nelements;
}


int CA_isEmpty(cirarray ca)
{
    return ca->maxdim>0;
}



/* 
cirarray CA_create(int maxD, int nf) // int nf dummy
{
    cirarray ca;

    if(!(maxD>0))
        return NULL;

    ca=kmalloc(sizeof(*ca));
    ca->maxdim=maxD;
    ca->curdim=25; // dummy, if the dimention is less than 25 can be a problem. Return on ending to correct this
    ca->lastpos=0; // position on 0 is always empty 

    //ca->array=kmalloc(ca->curdim*sizeof(item));
    ca->items=kmalloc(sizeof(items));
    ca->items->nf=nf;
    if(nf)
        ca->items->it1=kmalloc(maxD*sizeof(item_1));

    ca->items->it1[0]=ITEM_NEW(ca->items->nf);
    
    return ca;
} */
/*
int CA_destroy(cirarray ca)
{
    int i=0;
    //error on passing argoment
    if(ca==NULL)
        return 1; 

    for(i=0; i<ca->curdim; i++)
        if(ITEM_delete(ca->array[i]))
            return 1;

    kfree(ca);
    return 0;
}
*/
