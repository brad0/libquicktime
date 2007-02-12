/*-------------------------------------------------------------------------
 *
 * lqt_fseeko.c
 *	  64-bit versions of fseeko/ftello()
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 * $Header: /cvsroot/libquicktime/libquicktime/src/lqt_fseeko.c,v 1.3 2007/02/12 12:37:09 gmerlin Exp $
 *
 *-------------------------------------------------------------------------
 */

#ifdef	HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef	HAVE_FSEEKO

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/*
 *	On BSD/OS and NetBSD, off_t and fpos_t are the same.  Standards
 *	say off_t is an arithmetic type, but not necessarily integral,
 *	while fpos_t might be neither.
 */

int
lqt_fseeko(FILE *stream, off_t offset, int whence)
{
	off_t floc;
	struct stat filestat;

	switch (whence)
	{
		case SEEK_CUR:
			flockfile(stream);
			if (fgetpos(stream, &floc) != 0)
				goto failure;
			floc += offset;
			if (fsetpos(stream, &floc) != 0)
				goto failure;
			funlockfile(stream);
			return 0;
			break;
		case SEEK_SET:
			if (fsetpos(stream, &offset) != 0)
				return -1;
			return 0;
			break;
		case SEEK_END:
			flockfile(stream);
			if (fstat(fileno(stream), &filestat) != 0)
				goto failure;
			floc = filestat.st_size;
			if (fsetpos(stream, &floc) != 0)
				goto failure;
			funlockfile(stream);
			return 0;
			break;
		default:
			errno =	EINVAL;
			return -1;
	}

failure:
	funlockfile(stream);
	return -1;
}

off_t
lqt_ftello(FILE *stream)
{
	off_t floc;

	if (fgetpos(stream, &floc) != 0)
		return -1;
	return floc;
}

#endif
