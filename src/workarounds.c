
#define EX_INLINE

/* #include "libdv/vlc.h" */
/* #include "libdv/mmx.h" */

#include <quicktime/qtprivate.h>
#include <workarounds.h>

int64_t quicktime_add(int64_t a, int64_t b)
{
	return a + b;
}

int64_t quicktime_add3(int64_t a, int64_t b, int64_t c)
{
	return a + b + c;
}
