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

cirarray CA_create(int maxD, CAoperations ops)
{
    cirarray ca;

    if(!(maxD>0))
        return NULL;

    ca=kmalloc(sizeof(*ca));
    ca->maxdim=maxD;
    ca->curdim=heuristicDimension(maxD)+1; // dummy, if the dimention is less than 25 can be a problem. Return on ending to correct this
    ca->lastpos=0; // position on 0 is always empty 
    ca->nelements=0;
    ca->ops=ops;
    ca->item=kmalloc(ca->curdim*sizeof(*(ca->item)));
    bzero(ca->item, ca->curdim*sizeof(*(ca->item)));

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

static void CA_realloc(cirarray ca) 
{
    CAitem *buff;
    int i, pastdim=ca->curdim;

    ca->curdim=2*(ca->curdim) >= ca->maxdim ? ca->maxdim : 2*(ca->curdim);
    buff=kmalloc(ca->curdim*sizeof(*buff));

    for(i=1; i<pastdim; i++) {
        buff[i]=ca->ops.copyItem(ca->item[i]);
        ca->ops.freeItem(ca->item[i]);
    }
    
    kfree(ca->item);
    ca->item=buff;
    ca->lastpos=pastdim;
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

    if(!found) {
        if(ca->curdim < ca->maxdim) {
            CA_realloc(ca); // it's necerrary to allocate more space
            ca->item[ca->lastpos]=it;
            ca->nelements++;
        } else
            return 1; // no more space. Return error
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

