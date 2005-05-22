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
//#include <string.h>


/* util */

static int deg2frame(quicktime_t *file, float hdeg, float vdeg)
{
    return ((hdeg * abs(vdeg-90)) / 64800) * file->moov.udta.navg.columns * file->moov.udta.navg.rows * file->moov.udta.navg.loop_frames;
}

/* Is file a qtvr file */
int lqt_is_qtvr(quicktime_t  *file)
{
    if (file->moov.udta.is_qtvr) {
	if (quicktime_match_32(file->moov.udta.ctyp, "stna")) return QTVR_OBJ;
	if (quicktime_match_32(file->moov.udta.ctyp, "STpn")) return QTVR_PAN;
    }
    return 0;
}

/* write dummy node */
int lqt_qtvr_write_dummy_node(quicktime_t *file)
{
        int result = 0;
	uint8_t *dummy;
        quicktime_atom_t chunk_atom, atom;
        quicktime_trak_t *trak = file->moov.trak[lqt_qtvr_get_panorama_track(file)];
        dummy = calloc(sizeof(uint8_t), sizeof(quicktime_pHdr_t));
        quicktime_write_chunk_header(file, trak, &chunk_atom);
	quicktime_atom_write_header(file, &atom, "pHdr");
        result = !quicktime_write_data(file, dummy, sizeof(quicktime_pHdr_t));
	quicktime_atom_write_footer(file, &atom);
        quicktime_write_chunk_footer(file,
                                        trak,
                                        1,
                                        &chunk_atom,
                                        1);
        return result;
}

/* Type of qtvr to write */
int lqt_qtvr_set_type(quicktime_t  *file, int type, int width, int height, int duration, int time_scale, int scene_track)
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
	quicktime_trak_t *trak;
	
	file->moov.udta.ctyp[0] = 'S';
	file->moov.udta.ctyp[1] = 'T';
	file->moov.udta.ctyp[2] = 'p';
	file->moov.udta.ctyp[3] = 'n';
	file->moov.udta.is_qtvr = 1;
	
	trak = quicktime_add_track(file);
	if (time_scale <= 0) return 0;
	if (duration <= 0) return 0;
	if (scene_track >= file->total_vtracks && scene_track < 0) return 0;

	quicktime_trak_init_panorama(file, trak, width, height, duration, time_scale);
	/* set initial scene track*/
	lqt_qtvr_set_scene_track(file, scene_track);

	lqt_qtvr_write_dummy_node(file);

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
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return file->moov.udta.navg.rows;
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	return file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SNumFramesHeight;
    }
    return 0;
    
}

/* Number of columns */
int lqt_qtvr_get_columns(quicktime_t  *file)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return file->moov.udta.navg.columns;
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	return file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SNumFramesWidth;
    }
    return 0;

}

/* Starting position                                                          */
int lqt_qtvr_get_initial_pan(quicktime_t  *file, float *hpan, float *vpan)
{
    if (hpan != NULL ) *hpan = file->moov.udta.navg.initialHPan;
    if (vpan != NULL ) *vpan = file->moov.udta.navg.initialVPan;
    return deg2frame(file, file->moov.udta.navg.initialHPan, file->moov.udta.navg.initialVPan);
}

void lqt_qtvr_set_initial_pan(quicktime_t  *file, float hpan, float vpan)
{
    file->moov.udta.navg.initialHPan = hpan;
    file->moov.udta.navg.initialVPan = vpan;
}


/* extra settings */
void lqt_qtvr_get_extra_settings(quicktime_t *file, 
				   float *starthpan,
				   float *endhpan,
				   float *startvpan, 
				   float *endvpan, 
				   float *minzoom, 
				   float *maxzoom)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	if (starthpan != NULL ) *starthpan = file->moov.udta.navg.startHPan;
	if (endhpan != NULL ) *endhpan = file->moov.udta.navg.endHPan;
	if (startvpan != NULL ) *startvpan = file->moov.udta.navg.startVPan;
	if (endvpan != NULL ) *endvpan = file->moov.udta.navg.endVPan;
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	if (starthpan != NULL ) *starthpan  = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.HPanStart;
	if (endhpan != NULL ) *endhpan  = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.HPanEnd;
	if (startvpan != NULL ) *startvpan  = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.VPanStart;
	if (endvpan != NULL ) *endvpan  = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.VPanEnd;
	if (minzoom != NULL ) *minzoom  = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.MinZoom;
	if (maxzoom != NULL ) *maxzoom  = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.MaxZoom;
    }
}

void lqt_qtvr_set_extra_settings(quicktime_t  *file, float starthpan, float endhpan, float startvpan, float endvpan, float minzoom, float maxzoom)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	file->moov.udta.navg.startHPan = starthpan;
	file->moov.udta.navg.endHPan = endhpan;
	file->moov.udta.navg.startVPan = startvpan;
	file->moov.udta.navg.endVPan = endvpan;
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.HPanStart = starthpan;
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.HPanEnd = endhpan;
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.VPanStart = startvpan;
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.VPanEnd = endvpan;
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.MinZoom = minzoom;
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.MaxZoom = maxzoom;
    }
}


/* field of view                                                              */
float lqt_qtvr_get_fov(quicktime_t  *file)
{
    return file->moov.udta.navg.fieldofview;
}

int lqt_qtvr_set_fov(quicktime_t  *file, float fov)
{
    if (fov > 0) 
    { 
	file->moov.udta.navg.fieldofview = fov;
	return 1;
    }
    else return 0;
}



/* movietype */
int lqt_qtvr_get_movietype(quicktime_t  *file)
{
    return file->moov.udta.navg.movietype;
}

int lqt_qtvr_set_movietype(quicktime_t  *file, int movietype)
{
    if (movietype == QTVR_STANDARD_OBJECT || 
	movietype == QTVR_OLD_NAVIGABLE_MOVIE_SCENE || 
    	movietype == QTVR_OBJECT_IN_SCENE)
    {
	file->moov.udta.navg.movietype = movietype;
	return 0;
    }
    else return 1;
}


/* Set number of rows */
int lqt_qtvr_set_rows(quicktime_t  *file, short rows)
{
    if (rows > 0)
    {
	if (lqt_is_qtvr(file) == QTVR_OBJ) {
	    file->moov.udta.navg.rows = rows;
	return 1;
	}
	else
	if (lqt_is_qtvr(file) == QTVR_PAN) {
	    file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SNumFramesHeight = rows;
	    file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.NumFrames = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SNumFramesWidth * rows;
	return 1;
	}
    }
    return 0;
}

/* Set number of columns */
int lqt_qtvr_set_columns(quicktime_t  *file, short columns)
{
    if (columns > 0)
    {
	if (lqt_is_qtvr(file) == QTVR_OBJ) {
	    file->moov.udta.navg.columns = columns;
	return 1;
	}
	else
	if (lqt_is_qtvr(file) == QTVR_PAN) {
	    file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SNumFramesWidth = columns;
	    file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.NumFrames = file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SNumFramesHeight * columns;
	}	
    }
    return 0;
}

/* dimensions of the player window */
int lqt_qtvr_get_display_width(quicktime_t  *file)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return quicktime_video_width(file, 0);
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	return (int)file->moov.trak[lqt_qtvr_get_panorama_track(file)]->tkhd.track_width;
    }
    return 0;
}



int lqt_qtvr_set_display_width(quicktime_t  *file, int width)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return 1;
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->tkhd.track_width = (float)width;
    }
    return 0;
}

int lqt_qtvr_get_display_height(quicktime_t  *file)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return quicktime_video_height(file, 0);
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	return (int)file->moov.trak[lqt_qtvr_get_panorama_track(file)]->tkhd.track_height;
    }
    return 0;
}


int lqt_qtvr_set_display_height(quicktime_t *file, int height)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return 1;
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	file->moov.trak[lqt_qtvr_get_panorama_track(file)]->tkhd.track_height = height;
    }
    return 0;
}

/* get depth */
int lqt_qtvr_get_depth(quicktime_t  *file)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return quicktime_video_depth(file, 0);
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	return (int)file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SDepth;
    }
    return 0;
}

/* get/set full dimensions */
int lqt_qtvr_get_width(quicktime_t  *file)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return quicktime_video_width(file, 0);
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	return file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SWidth;
    }
    return 0;
}

int lqt_qtvr_get_height(quicktime_t  *file)
{
    if (lqt_is_qtvr(file) == QTVR_OBJ) {
	return quicktime_video_height(file, 0);
    }
    else
    if (lqt_is_qtvr(file) == QTVR_PAN) {
	return file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.SHeight;
    }
    return 0;
}


/* Panorama specific */
int lqt_qtvr_get_panorama_track(quicktime_t  *file)
{
    int i;
    for(i = 0; i < file->moov.total_tracks; i++) {

	if (quicktime_match_32(file->moov.trak[i]->mdia.hdlr.component_subtype, "STpn")) {
	    return i;
	}
    }
    return -1;
}


/* Panorama specific */
int lqt_qtvr_set_scene_track(quicktime_t  *file, int track)
{
    int ptrack = lqt_qtvr_get_panorama_track(file);
    
    if (ptrack != -1 && file->total_vtracks > track) {
	/* reset the prevous vtracks flags if required */
	if (file->moov.trak[ptrack]->mdia.minf.stbl.stsd.table->pano.STrack != 0) 
	    	file->moov.trak[lqt_track_from_id(file, file->moov.trak[ptrack]->mdia.minf.stbl.stsd.table->pano.STrack)]->tkhd.flags = 15;
	
	file->moov.trak[ptrack]->mdia.minf.stbl.stsd.table->pano.STrack = file->vtracks[track].track->tkhd.track_id;
	file->vtracks[track].track->tkhd.flags = 14; /* deactivate this vtrack */
	return 1;
    }
    return 0;
}

/* Panorama specific */
int lqt_qtvr_get_scene_track(quicktime_t  *file)
{
    if (lqt_qtvr_get_panorama_track(file) != -1) {
	return file->moov.trak[lqt_qtvr_get_panorama_track(file)]->mdia.minf.stbl.stsd.table->pano.STrack;
    }
    return 0;
}

/* get track from track id */
int lqt_track_from_id(quicktime_t *file, int track_id)
{
    int i;
    for(i = 0; i < file->moov.total_tracks; i++) {
	if (file->moov.trak[i]->tkhd.track_id == track_id ) return i;
    }
    return -1;
}



