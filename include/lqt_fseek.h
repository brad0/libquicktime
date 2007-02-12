#ifdef	HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef	HAVE_FSEEKO
/* 
 * The compatibility file src/fseeko.c is being used.  Define the prototypes
 * in this private include file.
*/
int lqt_fseeko(FILE *, off_t, int );
off_t lqt_ftello(FILE *);

#define fseeko(a,b,c) lqt_fseeko(a,b,c)
#define ftello(a) lqt_ftello(a)
#else
#include <stdio.h>
#endif
