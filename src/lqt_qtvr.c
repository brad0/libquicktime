#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <quicktime/quicktime.h>
#include <quicktime/lqt.h>
#include <lqt_codecinfo_private.h>

#include <lqt_private.h>
#include <funcprotos.h>
#include <lqt_fseek.h>
#include <sys/stat.h>
#include <string.h>

/* Is file a qtvr file */
int lqt_is_qtvr(quicktime_t  *file)
{
    if (file->moov.udta.is_qtvr) {
	if (strcmp(file->moov.udta.ctyp, "stna")) return QTVR_OBJ;
	if (strcmp(file->moov.udta.ctyp, "STpn")) return QTVR_PAN;
    }
    return 0;
}

/* Type of qtvr to write */
int lqt_qtvr_set_type(quicktime_t  *file, int type)
{
    if (type == QTVR_OBJ) {
	file->moov.udta.ctyp[0] = 's';
	file->moov.udta.ctyp[1] = 't';
	file->moov.udta.ctyp[2] = 'n';
	file->moov.udta.ctyp[3] = 'a';
	file->moov.udta.is_qtvr = 1;
	file->moov.udta.navg.loop_dur = lqt_frame_duration(file, 0, NULL);
	return 1;
    }
    if (type == QTVR_PAN) {
	file->moov.udta.ctyp[0] = 'S';
	file->moov.udta.ctyp[1] = 'T';
	file->moov.udta.ctyp[2] = 'p';
	file->moov.udta.ctyp[3] = 'n';
	file->moov.udta.is_qtvr = 1;
	return 1;
    }
    file->moov.udta.is_qtvr = 0;
    return 0;
}

/* Number of frames in loop */
int lqt_qtvr_get_loop_frames(quicktime_t  *file)
{
    return file->moov.udta.navg.loop_frames;
}

/* Number of rows */
int lqt_qtvr_get_rows(quicktime_t  *file)
{
    return file->moov.udta.navg.rows;
}

/* Number of columns */
int lqt_qtvr_get_columns(quicktime_t  *file)
{
    return file->moov.udta.navg.columns;
}

/* Set number of rows */
int lqt_qtvr_set_rows(quicktime_t  *file, short rows)
{
    if (rows > 0)
    {
	file->moov.udta.navg.rows = rows;
	return 1;
    }
    return 0;
}

/* Set number of columns */
int lqt_qtvr_set_columns(quicktime_t  *file, short columns)
{
    if (columns > 0)
    {
	file->moov.udta.navg.columns = columns;
	return 1;
    }
    return 0;
}

