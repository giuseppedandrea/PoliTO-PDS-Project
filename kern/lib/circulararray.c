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

static unsigned int heuristicDimension(unsigned int maxD)
{
    unsigned int count = 0, dim=maxD;
    while (dim)
    {
        count++;
        dim >>= 1;
    }

    return (maxD)*count/(16*sizeof(int)) > 300 ? 300: (maxD)*count/(16*sizeof(int));
}

static void CA_realloc(cirarray ca) 
{
    CAitem *buff;
    int i, pastdim=ca->curdim, propdim=2*(ca->curdim)+1; 

    /* Dimension proposed is 2 times the last */
    ca->curdim=propdim >= ca->maxdim ? ca->maxdim : propdim; 
    buff=kmalloc(ca->curdim*sizeof(CAitem));
    bzero(buff, ca->curdim*sizeof(CAitem));

    for(i=1; i<pastdim; i++) {
        buff[i]=ca->ops.copyItem(ca->item[i]);
        ca->ops.freeItem(ca->item[i]);
    }
    
    kfree(ca->item);
    ca->item=buff;
    ca->lastpos=pastdim;
}

cirarray CA_create(int maxD, CAoperations ops)
{
    cirarray ca;

    if(!(maxD>0))
        return NULL;

    ca=kmalloc(sizeof(*ca));
    ca->maxdim=maxD+1;
    ca->curdim=heuristicDimension(maxD)+1; 
    ca->lastpos=0; // position on 0 is always empty 
    ca->nelements=0;
    ca->ops=ops;
    ca->item=kmalloc(ca->curdim*sizeof(CAitem));
    bzero(ca->item, ca->curdim*sizeof(CAitem));

    return ca;
}

int CA_destroy(cirarray ca)
{
    int i;

    if(ca==NULL)
        return 1; //passing error
    if((ca->nelements)!=0) //CA is empty
        for(i=1; i<ca->curdim; i++)
            if(ca->item[i]!=NULL)
                ca->ops.freeItem(ca->item[i]);  
    
    kfree(ca->item);
    kfree(ca);
    
    return 0;
}

int CA_add(cirarray ca, CAitem it)
{
    int i=0, found=0, pos=0;

    if(ca==NULL || it==NULL)
        return 0;
    
    i=ca->lastpos+1;

    if(i>=ca->curdim)
        i=1;
    
    while (i != ca->lastpos) {
        if (ca->item[i] == NULL) {
            ca->item[i]=it;
            ca->lastpos=i;
            ca->nelements++;
            found=1;
            pos=i;
            break;
        } 
        i++;
        if(i>ca->curdim)
            i=1;
    }

    if(!found) {
        if(ca->curdim < ca->maxdim) {
            CA_realloc(ca); // it's necerrary to allocate more space
            ca->item[ca->lastpos]=it;
            pos=ca->lastpos;
            ca->nelements++;
        } else
            return 0; // no more space. Return error
    }

    return pos;
}

int CA_set(cirarray ca, CAitem it, int pos) 
{

    if(ca==NULL || it==NULL || pos<=0) 
        return 1;

    if(pos>ca->curdim)
        return 1;
    
    ca->item[pos]=it;
    return 0;
}

CAitem CA_get_byIndex(cirarray ca, int pos)
{
    if(ca==NULL || pos<=0)
        return NULL;
    
    if(ca->item[pos]==NULL)
        return NULL;

    return ca->item[pos];
}

int CA_remove_byIndex(cirarray ca, int pos)
{
    if(ca==NULL || pos<=0)
        return 1;
    
    if(ca->item[pos]==NULL)
        return 1;

    ca->ops.freeItem(ca->item[pos]);
    ca->item[pos]=NULL;
    ca->lastpos=pos-1;
    ca->nelements--;
    return 0;
}

int* CA_get(cirarray ca, CAkey key)
{
    int *v, *buff;
    int i, j=1, k=0;

    if(ca==NULL || key==NULL)
        return NULL;

    v=kmalloc(ca->curdim*sizeof(int));
    bzero(v, ca->curdim*sizeof(int));

    for(i=1; i<ca->curdim; i++)
        if(ca->item[i]!=NULL)
            if(ca->ops.cmpItem(ca->ops.getItemKey(ca->item[i]), key)==0)
                v[k++]=i;
    
    buff=kmalloc((k+1)*sizeof(int));
    buff[0]=k;
    for(i=0; i<k; i++)
        buff[j++]=v[i];

    kfree(v);

    return buff;
}

int CA_remove(cirarray ca, CAkey key)
{
    int *v, *buff;
    int i, k=0;

    if(ca==NULL || key==NULL)
        return 1;

    v=kmalloc(ca->curdim*sizeof(int));
    bzero(v, ca->curdim*sizeof(int));

    for(i=1; i<ca->curdim; i++)
        if(ca->item[i]!=NULL)
            if(ca->ops.cmpItem(ca->ops.getItemKey(ca->item[i]), key)==0)
                v[k++]=i;
    
    if(k<=0) {
        kfree(v);
        return 1; // no occurance, return error
    }

    buff=kmalloc(k*sizeof(int));
    for(i=0; i<k; i++)
        buff[i]=v[i];

    kfree(v);

    for(i=0; i<k; i++) 
        if(CA_remove_byIndex(ca, buff[i]))
            return 1;

    return 0;
}

cirarray CA_duplicate(cirarray src)
{
    cirarray dst;
    int i;

    if(src==NULL)
        return NULL;

    dst=kmalloc(sizeof(*dst));
    dst->ops=src->ops;
    dst->curdim=src->curdim;
    dst->lastpos=src->lastpos;
    dst->nelements=src->nelements;
    dst->maxdim=src->maxdim;
    dst->item=kmalloc(dst->curdim*sizeof(CAitem));
    bzero(dst->item, dst->curdim*sizeof(CAitem));


    for(i=1; i<dst->curdim; i++)
        if(src->item[i]!=NULL)
            dst->item[i]=dst->ops.copyItem(src->item[i]);
    
    return dst;
}

int CA_stamp(cirarray ca)
{
    int i;

    if(ca==NULL)
        return 1;

    kprintf("Database:\n");
    for(i=1; i<ca->curdim; i++)
        if(ca->item[i]!=NULL)
            {
                kprintf("POS[%d]= ", i);    
                ca->ops.coutItem(ca->item[i]);
                kprintf("\n");
            }
    return 0;
}

int CA_size(cirarray ca)
{
    return ca->curdim;
}


int CA_isEmpty(cirarray ca)
{
    return ca->nelements==0;
}