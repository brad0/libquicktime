#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_minf_init(quicktime_minf_t *minf)
{
	minf->is_video = minf->is_audio = 0;
	minf->is_panorama = 0;
	minf->is_qtvr = 0;
	minf->is_object = 0;
	quicktime_vmhd_init(&(minf->vmhd));
	quicktime_smhd_init(&(minf->smhd));
	quicktime_gmhd_init(&(minf->gmhd));
	quicktime_hdlr_init(&(minf->hdlr));
	quicktime_dinf_init(&(minf->dinf));
	quicktime_stbl_init(&(minf->stbl));
}


void quicktime_minf_init_qtvr(quicktime_t *file,
								quicktime_minf_t *minf,
								int track_type,
								int width,
								int height,
								int frame_duration)
{
	minf->is_qtvr = track_type;
	quicktime_stbl_init_qtvr(file, &(minf->stbl), track_type, width, height, frame_duration);
	quicktime_hdlr_init_data(&(minf->hdlr));
	quicktime_dinf_init_all(&(minf->dinf));
}


void quicktime_minf_init_panorama(quicktime_t *file,
								quicktime_minf_t *minf,
								int width,
								int height,
								int frame_duration)
{
	minf->is_panorama = 1;
	quicktime_stbl_init_panorama(file, &(minf->stbl), width, height, frame_duration);
	quicktime_hdlr_init_data(&(minf->hdlr));
	quicktime_dinf_init_all(&(minf->dinf));
}


void quicktime_minf_init_video(quicktime_t *file, 
								quicktime_minf_t *minf, 
								int frame_w,
								int frame_h, 
                                                                int frame_duration,
                                                                int time_scale, 
								char *compressor)
{
	minf->is_video = 1;
//printf("quicktime_minf_init_video 1\n");
	quicktime_vmhd_init_video(file, &(minf->vmhd), frame_w, frame_h, frame_duration, time_scale);
//printf("quicktime_minf_init_video 1 %d %f\n", time_scale, frame_rate);
	quicktime_stbl_init_video(file, &(minf->stbl), frame_w, frame_h, frame_duration, time_scale, compressor);
//printf("quicktime_minf_init_video 2\n");
	quicktime_hdlr_init_data(&(minf->hdlr));
//printf("quicktime_minf_init_video 1\n");
	quicktime_dinf_init_all(&(minf->dinf));
//printf("quicktime_minf_init_video 2\n");
}

void quicktime_minf_init_audio(quicktime_t *file, 
							quicktime_minf_t *minf, 
							int channels, 
							int sample_rate, 
							int bits, 
							char *compressor)
{
	minf->is_audio = 1;
/* smhd doesn't store anything worth initializing */
	quicktime_stbl_init_audio(file, &(minf->stbl), channels, sample_rate, bits, compressor);
	quicktime_hdlr_init_data(&(minf->hdlr));
	quicktime_dinf_init_all(&(minf->dinf));
}

void quicktime_minf_delete(quicktime_minf_t *minf)
{
	quicktime_vmhd_delete(&(minf->vmhd));
	quicktime_smhd_delete(&(minf->smhd));
	quicktime_gmhd_delete(&(minf->gmhd));
	quicktime_dinf_delete(&(minf->dinf));
	quicktime_stbl_delete(&(minf->stbl));
	quicktime_hdlr_delete(&(minf->hdlr));
}

void quicktime_minf_dump(quicktime_minf_t *minf)
{
	printf("   media info\n");
	printf("    is_audio     %d\n", minf->is_audio);
	printf("    is_audio_vbr %d\n", minf->is_audio_vbr);
	printf("    is_video     %d\n", minf->is_video);
	if(minf->is_audio) quicktime_smhd_dump(&(minf->smhd));
	if(minf->is_video) quicktime_vmhd_dump(&(minf->vmhd));
	if(minf->has_baseheader) quicktime_gmhd_dump(&(minf->gmhd));
	quicktime_hdlr_dump(&(minf->hdlr));
	quicktime_dinf_dump(&(minf->dinf));
	quicktime_stbl_dump(minf, &(minf->stbl));
}

int quicktime_read_minf(quicktime_t *file, quicktime_trak_t *trak,
                        quicktime_minf_t *minf, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;
	do
	{
		quicktime_atom_read_header(file, &leaf_atom);
//printf("quicktime_read_minf 1\n");

/* mandatory */
		if(quicktime_atom_is(&leaf_atom, "vmhd"))
			{ minf->is_video = 1; quicktime_read_vmhd(file, &(minf->vmhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "smhd"))
			{ minf->is_audio = 1; quicktime_read_smhd(file, &(minf->smhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "gmhd"))
			{ minf->has_baseheader = 1; quicktime_read_gmhd(file, &(minf->gmhd), &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "hdlr"))
			{ 

				quicktime_read_hdlr(file, &(minf->hdlr));
				/* Main Actor doesn't write component name */
				quicktime_atom_skip(file, &leaf_atom);
			}
		else
		if(quicktime_atom_is(&leaf_atom, "dinf"))
			{ quicktime_read_dinf(file, &(minf->dinf), &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "stbl"))
			{ quicktime_read_stbl(file, minf, &(minf->stbl), &leaf_atom); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);

        /* Finalize stsd */
        quicktime_finalize_stsd(file, trak, &(minf->stbl.stsd));
          
        
        if(minf->is_audio && (minf->stbl.stsd.table[0].compression_id == -2))
          minf->is_audio_vbr = 1;
	return 0;
}

void quicktime_write_minf(quicktime_t *file, quicktime_minf_t *minf)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "minf");

	if(minf->is_video) quicktime_write_vmhd(file, &(minf->vmhd));
	if(minf->is_audio) quicktime_write_smhd(file, &(minf->smhd));
	if(minf->is_panorama) quicktime_write_gmhd(file, &(minf->gmhd));
	if(minf->is_qtvr != 0) quicktime_write_gmhd(file, &(minf->gmhd));
	
	quicktime_write_hdlr(file, &(minf->hdlr));
	quicktime_write_dinf(file, &(minf->dinf));
	quicktime_write_stbl(file, minf, &(minf->stbl));

	quicktime_atom_write_footer(file, &atom);
}
