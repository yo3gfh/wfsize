
#ifndef _MEM_H
#define _MEM_H

#include <windows.h>

// allign at 4k boundaries
#define ALIGN_4K(x) ((((UINT_PTR)(x))+4096+1)&(~4095)) 

void    * alloc_and_zero_mem    ( size_t nbytes );
void    * realloc_and_zero_mem  ( void * buf, size_t mbytes );
BOOL    free_mem                ( void * buf );

#endif
