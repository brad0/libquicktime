#include "config.h"

#ifndef	HAVE_FSEEKO
/* 
 * The compatibility file src/fseeko.c is being used.  Define the prototypes
 * in this private include file.
*/
int lqt_fseeko(FILE *, off_t, int );
int lqt_fseeko64(FILE *, off_t, int );
off_t lqt_ftello(FILE *);
off_t lqt_ftello64(FILE *);

#define fseeko(a,b,c) lqt_fseeko(a,b,c)
#define fseeko64(a,b,c) lqt_fseeko64(a,b,c)
#define ftello(a) lqt_ftello(a)
#define ftello64(a) lqt_ftello64(a)
#endif

#define FTELL ftello64
#define FSEEK fseeko64

