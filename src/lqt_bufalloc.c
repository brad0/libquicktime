/*
 *
 * lqt_bufalloc.c
 *	Aligned buffer allocation routine. 
 *
 * This routine should be used everywhere that a SIMD (MMX/MMX2/SSE/Altivec)
 * routine may operate on allocated buffers.  SIMD routines require that the
 * buffers be aligned on a 16 byte boundary (SSE/SSE2 require 64 byte 
 * alignment).
 *
 * $Header: /cvsroot/libquicktime/libquicktime/src/lqt_bufalloc.c,v 1.3 2005/08/06 23:23:38 sms00 Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

/*
 * Allocate memory aligned to suit SIMD 
*/

#define powerof2(x)     ((((x)-1)&(x))==0)

#if	!defined(HAVE_POSIX_MEMALIGN)

static int
posix_memalign(void **ptr, size_t alignment, size_t size)
	{
	void *mem;

	if	(alignment % sizeof (void *) != 0 || !powerof2(alignment) != 0)
		return(EINVAL);
	mem = malloc((size + alignment - 1) & ~(alignment - 1));
	if	(mem != NULL)
		{
		*ptr = mem;
		return(0);
		}
	return(ENOMEM);
	}
#endif

#if	!defined(HAVE_MEMALIGN)
static void *
memalign(size_t alignment, size_t size)
	{

	if 	(alignment % sizeof (void *) || !powerof2(alignment))
		{
		errno = EINVAL;
		return(NULL);
		}
	return(malloc((size + alignment - 1) & ~(alignment - 1)));
	}
#endif

void *lqt_bufalloc(size_t size)
	{
	static size_t simd_alignment = 16;
	static int bufalloc_init = 0;
	int  pgsize;
	void *buf = NULL;

	if	(!bufalloc_init)
		{
#ifdef ARCH_X86 
		simd_alignment = 64;	/* X86 requires 64 for SSE */
		bufalloc_init = 1;
#endif		
		}
		
	pgsize = sysconf(_SC_PAGESIZE);
/*
 * If posix_memalign fails it could be a broken glibc that caused the error,
 * so try again with a page aligned memalign request
*/
	if	(posix_memalign( &buf, simd_alignment, size))
		buf = memalign(pgsize, size);
	if	(buf && ((size_t)buf & (simd_alignment - 1)))
		{
		free(buf);
		buf = memalign(pgsize, size);
		}
	if	(buf == NULL)
		fprintf(stderr, "lqt_bufalloc: malloc of %d bytes failed", (int)size);
	else
		memset(buf, '\0', size);

	if	((size_t)buf & (simd_alignment - 1))
		fprintf(stderr, "lqt_bufalloc: could not allocate %d bytes aligned on a %d byte boundary", (int)size, (int)simd_alignment);
	return buf;
	}
