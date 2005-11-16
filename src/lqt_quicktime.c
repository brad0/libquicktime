#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <quicktime/colormodels.h>
#include <quicktime/quicktime.h>
#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <lqt_codecinfo_private.h>

#include <lqt_private.h>
#include <funcprotos.h>
#include <lqt_fseek.h>
#include <sys/stat.h>
#include <string.h>


static int64_t get_file_length(quicktime_t *file)
{
        int64_t current_pos, total_bytes;
        current_pos = ftello(file->stream);
        fseeko(file->stream, 0, SEEK_END);
        total_bytes = ftello(file->stream);
        fseeko(file->stream, current_pos, SEEK_CUR);
        return total_bytes;
}

int lqt_fileno(quicktime_t *file)
  {
  FILE *fp;

  fp = file->stream;
  return(fileno(fp));
  }

int quicktime_make_streamable(char *in_path, char *out_path)
{
	quicktime_t file, *old_file, new_file;
	int moov_exists = 0, mdat_exists = 0, result, atoms = 1;
	int64_t mdat_start = 0, mdat_size = 0;
	quicktime_atom_t leaf_atom;
	int64_t moov_length = 0;
	
	memset(&new_file,0,sizeof(quicktime_t));
	
	quicktime_init(&file);

/* find the moov atom in the old file */
	
	if(!(file.stream = fopen(in_path, "rb")))
	{
		perror("quicktime_make_streamable");
		return 1;
	}

	file.total_length = get_file_length(&file);

/* get the locations of moov and mdat atoms */
	do
	{
/*printf("%x\n", quicktime_position(&file)); */
		result = quicktime_atom_read_header(&file, &leaf_atom);

		if(!result)
		{
			if(quicktime_atom_is(&leaf_atom, "moov"))
			{
				moov_exists = atoms;
				moov_length = leaf_atom.size;
			}
			else
			if(quicktime_atom_is(&leaf_atom, "mdat"))
			{
				mdat_start = quicktime_position(&file) - HEADER_LENGTH;
				mdat_size = leaf_atom.size;
				mdat_exists = atoms;
			}

			quicktime_atom_skip(&file, &leaf_atom);

			atoms++;
		}
	}while(!result && quicktime_position(&file) <
               file.total_length);
	fclose(file.stream);

	if(!moov_exists)
	{
		printf("quicktime_make_streamable: no moov atom\n");
		return 1;
	}

	if(!mdat_exists)
	{
		printf("quicktime_make_streamable: no mdat atom\n");
		return 1;
	}

/* copy the old file to the new file */
	if(moov_exists && mdat_exists)
	{
/* moov wasn't the first atom */
		if(moov_exists > 1)
		{
			uint8_t *buffer;
			int64_t buf_size = 1000000;

			result = 0;

/* read the header proper */
			if(!(old_file = quicktime_open(in_path, 1, 0)))
			{
				return 1;
			}

			quicktime_shift_offsets(&(old_file->moov), moov_length+8);

/* open the output file */
			
			
			if(!(new_file.stream = fopen(out_path, "wb")))
			{
				perror("quicktime_make_streamable");
				result =  1;
			}
			else
			{
/* set up some flags */
				new_file.wr = 1;
				new_file.rd = 0;
				new_file.cpus = 1;
		      	new_file.presave_buffer = calloc(1, QUICKTIME_PRESAVE);

				quicktime_write_moov(&new_file, &(old_file->moov));
				
				quicktime_set_position(&new_file, moov_length);

        	    quicktime_atom_write_header64(&new_file, 
                                         
&(new_file.mdat.atom), 
                                          "mdat");
				
				quicktime_set_position(old_file, mdat_start);

				if(!(buffer = calloc(1, buf_size)))
				{
					result = 1;
					printf("quicktime_make_streamable: out of memory\n");
				}
				else
				{
					while(quicktime_position(old_file) < mdat_start +
mdat_size && !result)
					{
						if(quicktime_position(old_file) + buf_size >
mdat_start + mdat_size)
							buf_size = mdat_start + mdat_size -
quicktime_position(old_file);

						if(!quicktime_read_data(old_file, buffer, buf_size))
result = 1;
						if(!result)
						{
							if(!quicktime_write_data(&new_file, buffer,
buf_size)) result = 1;
 
						}
					}
					free(buffer);
				}

				quicktime_atom_write_footer(&new_file, 
                                         
&(new_file.mdat.atom));
			
				
       			if(new_file.presave_size)
        		{
        			quicktime_fseek(&new_file,
new_file.presave_position - new_file.presave_size);
                	fwrite(new_file.presave_buffer, 1,
new_file.presave_size, new_file.stream);
                	new_file.presave_size = 0;
        		}
				free(new_file.presave_buffer);
				fclose(new_file.stream);
			}
			quicktime_close(old_file);
		}
		else
		{
			printf("quicktime_make_streamable: header already at 0 offset\n");
			return 0;
		}
	}
	
	return 0;
}




void lqt_set_audio_parameter(quicktime_t *file,int stream, char *key,void *value)
  {
  quicktime_codec_t *codec = (quicktime_codec_t*)file->atracks[stream].codec;
  if(codec->set_parameter) codec->set_parameter(file, stream, key, value);
  }

void lqt_set_video_parameter(quicktime_t *file,int stream, char *key,void *value)
  {
  quicktime_codec_t *codec = (quicktime_codec_t*)file->vtracks[stream].codec;
  if(codec->set_parameter) codec->set_parameter(file, stream, key, value);
  }

void quicktime_set_parameter(quicktime_t *file, char *key, void *value)
{
	int i;
	for(i = 0; i < file->total_vtracks; i++)
	{
        lqt_set_video_parameter(file,i, key,value);
	}

	for(i = 0; i < file->total_atracks; i++)
	{
        lqt_set_audio_parameter(file,i, key,value);
	}
}



void quicktime_set_jpeg(quicktime_t *file, int quality, int use_float)
{
	quicktime_set_parameter( file, "jpeg_quality", &quality );
	quicktime_set_parameter( file, "jpeg_usefloat", &use_float );

	/*
	  int i;
	  char *compressor;

	  for(i = 0; i < file->total_vtracks; i++)
	  {
	  if(quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_JPEG) ||
	  quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_MJPA) ||
	  quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_RTJ0))
	  {
	  //quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[i].codec)->priv;
	  //mjpeg_set_quality(codec->mjpeg, quality);
	  //mjpeg_set_float(codec->mjpeg, use_float);
	  }
	  }
	*/
}


void quicktime_set_copyright(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.copyright), &(file->moov.udta.copyright_len), string);
}

void quicktime_set_name(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.name), &(file->moov.udta.name_len), string);
}

void quicktime_set_info(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.info), &(file->moov.udta.info_len), string);
}

char* quicktime_get_copyright(quicktime_t *file)
{
	return file->moov.udta.copyright;
}

char* quicktime_get_name(quicktime_t *file)
{
	return file->moov.udta.name;
}

char* quicktime_get_info(quicktime_t *file)
{
	return file->moov.udta.info;
}

/* Extended metadata support */

void lqt_set_album(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.album), &(file->moov.udta.album_len), string);
}

void lqt_set_artist(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.artist), &(file->moov.udta.artist_len), string);
}

void lqt_set_genre(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.genre), &(file->moov.udta.genre_len), string);
}

void lqt_set_track(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.track), &(file->moov.udta.track_len), string);
}

void lqt_set_comment(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.comment), &(file->moov.udta.comment_len), string);
}

void lqt_set_author(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.author), &(file->moov.udta.author_len), string);
}

char * lqt_get_album(quicktime_t * file)
{
        return file->moov.udta.album;
}

char * lqt_get_artist(quicktime_t * file)
{
        return file->moov.udta.artist;
}

char * lqt_get_genre(quicktime_t * file)
{
        return file->moov.udta.genre;
}

char * lqt_get_track(quicktime_t * file)
{
        return file->moov.udta.track;
}

char * lqt_get_comment(quicktime_t *file)
{
        return file->moov.udta.comment;
}

char * lqt_get_author(quicktime_t *file)
{
        return file->moov.udta.author;
}

int quicktime_video_tracks(quicktime_t *file)
{
	int i, result = 0;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		if(file->moov.trak[i]->mdia.minf.is_video) result++;
	}
	return result;
}

int quicktime_audio_tracks(quicktime_t *file)
{
	int i, result = 0;
	quicktime_minf_t *minf;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		minf = &(file->moov.trak[i]->mdia.minf);
		if(minf->is_audio)
			result++;
	}
	return result;
}

int lqt_add_audio_track(quicktime_t *file,
                        int channels, long sample_rate, int bits, lqt_codec_info_t * codec_info)
  {
  quicktime_trak_t *trak;
  char * compressor = codec_info->fourccs[0];

  /* Fake the bits parameter for some formats. */
  if(quicktime_match_32(compressor, QUICKTIME_ULAW) ||
     quicktime_match_32(compressor, QUICKTIME_IMA4))
    bits = 16;
  else if(quicktime_match_32(compressor, QUICKTIME_RAW))
    bits = 8;

  file->atracks = realloc(file->atracks, (file->total_atracks+1)*sizeof(quicktime_audio_map_t));
  memset(&(file->atracks[file->total_atracks]), 0, sizeof(quicktime_audio_map_t));

  trak = quicktime_add_track(file);
  quicktime_trak_init_audio(file, trak, channels,
                            sample_rate, bits, compressor);
  
  quicktime_init_audio_map(&(file->atracks[0]), trak, file->wr,
                           codec_info);
  file->atracks[file->total_atracks].track = trak;
  file->atracks[file->total_atracks].channels = channels;
  file->atracks[file->total_atracks].current_position = 0;
  file->atracks[file->total_atracks].current_chunk = 1;
  lqt_set_default_audio_parameters(file, file->total_atracks);
  file->total_atracks++;
  return 0;
  }

int lqt_set_audio(quicktime_t *file, int channels,
                  long sample_rate,  int bits,
                  lqt_codec_info_t * codec_info)
  {
  lqt_add_audio_track(file, channels, sample_rate, bits, codec_info);
  return 0;
  }

int quicktime_set_audio(quicktime_t *file, 
						int channels,
						long sample_rate,
						int bits,
						char *compressor)
{
        lqt_codec_info_t ** info;
        info = lqt_find_audio_codec(compressor, 1);
        lqt_set_audio(file, channels, sample_rate, bits, *info);
        lqt_destroy_codec_info(info);
        return 1;   /* Return the number of tracks created */
}

int lqt_set_video(quicktime_t *file, 
                  int tracks, 
                  int frame_w, 
                  int frame_h,
                  int frame_duration,
                  int timescale,
                  lqt_codec_info_t * info)
  {
	int i;

        for(i = 0; i < tracks; i++)
          lqt_add_video_track(file, frame_w, frame_h, frame_duration, timescale, info);
        return 0;
}

int quicktime_set_video(quicktime_t *file, 
                        int tracks, 
                        int frame_w, 
                        int frame_h,
                        double frame_rate,
                        char *compressor)
  {
        lqt_codec_info_t ** info;
        int timescale, frame_duration;
        timescale = quicktime_get_timescale(frame_rate);
        frame_duration = (int)((double)(timescale)/frame_rate+0.5);
        info = lqt_find_video_codec(compressor, 1);
        lqt_set_video(file, tracks, frame_w, frame_h, frame_duration, timescale, *info);
        lqt_destroy_codec_info(info);
        return 0;
}


int lqt_add_video_track(quicktime_t *file,
                        int frame_w, int frame_h,
                        int frame_duration, int timescale,
                        lqt_codec_info_t * info)
  {
        char * compressor = info->fourccs[0];
	quicktime_trak_t *trak;
        if(!file->total_vtracks)
          quicktime_mhvd_init_video(file, &(file->moov.mvhd), timescale);
        file->vtracks = realloc(file->vtracks, (file->total_vtracks+1) * sizeof(quicktime_video_map_t));
        memset(&(file->vtracks[file->total_vtracks]), 0, sizeof(quicktime_video_map_t));
        trak = quicktime_add_track(file);

        file->total_vtracks++;
        
        quicktime_trak_init_video(file, trak, frame_w, frame_h, frame_duration, timescale, compressor);
	quicktime_init_video_map(&(file->vtracks[file->total_vtracks-1]), trak, file->wr, info);
        lqt_set_default_video_parameters(file, file->total_vtracks-1);

        /* Get encoding colormodel */
        ((quicktime_codec_t*)(file->vtracks[file->total_vtracks-1].codec))->encode_video(file, (uint8_t**)0, file->total_vtracks-1);
        file->vtracks[file->total_vtracks-1].io_cmodel = file->vtracks[file->total_vtracks-1].stream_cmodel;

        lqt_get_default_rowspan(file->vtracks[file->total_vtracks-1].io_cmodel,
                                quicktime_video_width(file, file->total_vtracks-1),
                                &(file->vtracks[file->total_vtracks-1].io_row_span),
                                &(file->vtracks[file->total_vtracks-1].io_row_span_uv));

        file->vtracks[file->total_vtracks-1].stream_row_span = file->vtracks[file->total_vtracks-1].io_row_span;
        file->vtracks[file->total_vtracks-1].stream_row_span_uv = file->vtracks[file->total_vtracks-1].io_row_span_uv;
        return 0;
  }




void quicktime_set_framerate(quicktime_t *file, double framerate)
{
	int i;
	int new_time_scale, new_sample_duration;

	if(!file->wr)
	{
		fprintf(stderr, "quicktime_set_framerate shouldn't be called in read mode.\n");
		return;
	}

	new_time_scale = quicktime_get_timescale(framerate);
	new_sample_duration = (int)((float)new_time_scale / framerate + 0.5);

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.mdhd.time_scale = new_time_scale;
		file->vtracks[i].track->mdia.minf.stbl.stts.table[0].sample_duration = new_sample_duration;
	}
}
quicktime_trak_t* quicktime_add_track(quicktime_t *file)
{
        quicktime_moov_t *moov = &(file->moov);
        quicktime_trak_t *trak;
        int i;

        for(i = moov->total_tracks; i > 0; i--)
                moov->trak[i] = moov->trak[i - 1];
                                                                                                                  
        trak =
                moov->trak[0] =
                calloc(1, sizeof(quicktime_trak_t));
        quicktime_trak_init(trak);
        moov->total_tracks++;
                                                                                                                  
        for(i = 0; i < moov->total_tracks; i++)
                moov->trak[i]->tkhd.track_id = i + 1;
        moov->mvhd.next_track_id++;
        return trak;
}

/* ============================= Initialization functions */

int quicktime_init(quicktime_t *file)
{
	bzero(file, sizeof(quicktime_t));
//	quicktime_atom_write_header64(new_file, &file->mdat.atom, "mdat");
	quicktime_moov_init(&(file->moov));
	file->cpus = 1;
        //	file->color_model = BC_RGB888;
	return 0;
}

int quicktime_delete(quicktime_t *file)
{
	int i;
	if(file->total_atracks) 
	{
		for(i = 0; i < file->total_atracks; i++)
			quicktime_delete_audio_map(&(file->atracks[i]));
		free(file->atracks);
	}
	if(file->total_vtracks)
	{
		for(i = 0; i < file->total_vtracks; i++)
			quicktime_delete_video_map(&(file->vtracks[i]));
		free(file->vtracks);
	}
	file->total_atracks = 0;
	file->total_vtracks = 0;

        if(file->moov_data)
          free(file->moov_data);
        
        if(file->preload_size)
	{
		free(file->preload_buffer);
		file->preload_size = 0;
	}
        if(file->presave_buffer)
        {
                free(file->presave_buffer);
        }
        for(i = 0; i < file->total_riffs; i++)
        {
                quicktime_delete_riff(file, file->riff[i]);
        }

	quicktime_moov_delete(&(file->moov));
	quicktime_mdat_delete(&(file->mdat));
	return 0;
}

/* =============================== Optimization functions */

int quicktime_set_cpus(quicktime_t *file, int cpus)
{
	if(cpus > 0) file->cpus = cpus;
	return 0;
}

void quicktime_set_preload(quicktime_t *file, int64_t preload)
{
        file->preload_size = preload;
        if(file->preload_buffer) free(file->preload_buffer);
        file->preload_buffer = 0;
        if(preload)
                file->preload_buffer = calloc(1, preload);
        file->preload_start = 0;
        file->preload_end = 0;
        file->preload_ptr = 0;
}


int quicktime_get_timescale(double frame_rate)
{
	int timescale = 600;
/* Encode the 29.97, 23.976, 59.94 framerates */
	if(frame_rate - (int)frame_rate != 0) 
		timescale = (int)(frame_rate * 1001 + 0.5);
	else
	if((600 / frame_rate) - (int)(600 / frame_rate) != 0) 
			timescale = (int)(frame_rate * 100 + 0.5);
//printf("quicktime_get_timescale %f %d\n", 600 / frame_rate, (int)(600 / frame_rate));
	return timescale;
}

int quicktime_seek_end(quicktime_t *file)
{
	quicktime_set_position(file, file->mdat.atom.size + file->mdat.atom.start + HEADER_LENGTH * 2);
/*printf("quicktime_seek_end %ld\n", file->mdat.atom.size + file->mdat.atom.start); */
	quicktime_update_positions(file);
	return 0;
}

int quicktime_seek_start(quicktime_t *file)
{
	quicktime_set_position(file, file->mdat.atom.start + HEADER_LENGTH * 2);
	quicktime_update_positions(file);
	return 0;
}

long quicktime_audio_length(quicktime_t *file, int track)
{
	if(file->total_atracks > 0) 
		return quicktime_track_samples(file, file->atracks[track].track);

	return 0;
}

long quicktime_video_length(quicktime_t *file, int track)
{
/*printf("quicktime_video_length %d %d\n", quicktime_track_samples(file, file->vtracks[track].track), track); */
	if(file->total_vtracks > 0)
		return quicktime_track_samples(file, file->vtracks[track].track);
	return 0;
}

long quicktime_audio_position(quicktime_t *file, int track)
{
	return file->atracks[track].current_position;
}

long quicktime_video_position(quicktime_t *file, int track)
{
	return file->vtracks[track].current_position;
}

int quicktime_update_positions(quicktime_t *file)
{
/* Get the sample position from the file offset */
/* for routines that change the positions of all tracks, like */
/* seek_end and seek_start but not for routines that reposition one track, like */
/* set_audio_position. */

	int64_t mdat_offset = quicktime_position(file) - file->mdat.atom.start;
	int64_t sample, chunk, chunk_offset;
	int i;

	if(file->total_atracks)
	{
		sample = quicktime_offset_to_sample(file->atracks[0].track, mdat_offset);
		chunk = quicktime_offset_to_chunk(&chunk_offset, file->atracks[0].track, mdat_offset);
		for(i = 0; i < file->total_atracks; i++)
		{
			file->atracks[i].current_position = sample;
			file->atracks[i].current_chunk = chunk;
		}
	}

	if(file->total_vtracks)
	{
		sample = quicktime_offset_to_sample(file->vtracks[0].track, mdat_offset);
		chunk = quicktime_offset_to_chunk(&chunk_offset, file->vtracks[0].track, mdat_offset);
		for(i = 0; i < file->total_vtracks; i++)
		{
			file->vtracks[i].current_position = sample;
			file->vtracks[i].current_chunk = chunk;
		}
	}
	return 0;
}

int quicktime_set_audio_position(quicktime_t *file, int64_t sample, int track)
{
	if(track < file->total_atracks)
	{
#if 0 /* Old version (hopefully obsolete) */
		trak = file->atracks[track].track;
                file->atracks->current_position = sample;
                quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
		file->atracks[track].current_chunk = chunk;
		offset = quicktime_sample_to_offset(file, trak, sample);
		quicktime_set_position(file, offset);
#else
                file->atracks[track].current_position = sample;
                file->atracks[track].eof = 0;
                /* Codec will do the rest */
#endif
	}
	else
		fprintf(stderr, "quicktime_set_audio_position: track >= file->total_atracks\n");
        return 0;
}

int quicktime_set_video_position(quicktime_t *file, int64_t frame, int track)
{
	int64_t offset, chunk_sample, chunk;
	quicktime_trak_t *trak;

	if(track < file->total_vtracks)
	{
		trak = file->vtracks[track].track;
		file->vtracks[track].current_position = frame;
		quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, frame);
		file->vtracks[track].current_chunk = chunk;
		offset = quicktime_sample_to_offset(file, trak, frame);
		quicktime_set_position(file, offset);
                file->vtracks[track].timestamp =
                  quicktime_sample_to_time(&(trak->mdia.minf.stbl.stts),
                                           frame,
                                           &(file->vtracks[track].stts_index),
                                           &(file->vtracks[track].stts_count));
        }
	else
		fprintf(stderr, "quicktime_set_video_position: track >= file->total_vtracks\n");
	return 0;
}

void lqt_seek_video(quicktime_t * file, int track, int64_t time)
  {
  int64_t pos;
  int64_t offset, chunk_sample, chunk;
  quicktime_trak_t *trak;

  if(track >= file->total_vtracks)
    return;

  trak = file->vtracks[track].track;
  file->vtracks[track].timestamp = time;
  pos =
    quicktime_time_to_sample(&(trak->mdia.minf.stbl.stts),
                             &(file->vtracks[track].timestamp),
                             &(file->vtracks[track].stts_index),
                             &(file->vtracks[track].stts_count));

  file->vtracks[track].current_position = pos;
  quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, pos);
  file->vtracks[track].current_chunk = chunk;
  offset = quicktime_sample_to_offset(file, trak, pos);
  quicktime_set_position(file, offset);
  }

int quicktime_has_audio(quicktime_t *file)
{
	if(quicktime_audio_tracks(file)) return 1;
	return 0;
}

long quicktime_sample_rate(quicktime_t *file, int track)
{
	if(file->total_atracks)
		return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_rate;
	return 0;
}

int quicktime_audio_bits(quicktime_t *file, int track)
{
	if(file->total_atracks)
		return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_size;

	return 0;
}

char* quicktime_audio_compressor(quicktime_t *file, int track)
{
	return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int quicktime_track_channels(quicktime_t *file, int track)
{
	if(track < file->total_atracks)
		return file->atracks[track].channels;

	return 0;
}

int quicktime_channel_location(quicktime_t *file, int *quicktime_track, int *quicktime_channel, int channel)
{
	int current_channel = 0, current_track = 0;
	*quicktime_channel = 0;
	*quicktime_track = 0;
	for(current_channel = 0, current_track = 0; current_track < file->total_atracks; )
	{
		if(channel >= current_channel)
		{
			*quicktime_channel = channel - current_channel;
			*quicktime_track = current_track;
		}

		current_channel += file->atracks[current_track].channels;
		current_track++;
	}
	return 0;
}

int quicktime_has_video(quicktime_t *file)
{
	if(quicktime_video_tracks(file)) return 1;
	return 0;
}

int quicktime_video_width(quicktime_t *file, int track)
{
//	if(file->total_vtracks)
//          return file->vtracks[track].track->tkhd.track_width;
//	return 0;
  if((track < 0) || (track >= file->total_vtracks))
    return 0;
  return file->vtracks[track].track->mdia.minf.stbl.stsd.table->width;
  
}

int quicktime_video_height(quicktime_t *file, int track)
{
//	if(file->total_vtracks)
//          return file->vtracks[track].track->tkhd.track_height;
//	return 0;
  if((track < 0) || (track >= file->total_vtracks))
    return 0;
  return file->vtracks[track].track->mdia.minf.stbl.stsd.table->height;
}

int quicktime_video_depth(quicktime_t *file, int track)
{
	if(file->total_vtracks)
		return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].depth;
	return 0;
}

void quicktime_set_cmodel(quicktime_t *file, int colormodel)
  {
  int i;
  for(i = 0; i < file->total_vtracks; i++)
    file->vtracks[i].io_cmodel = colormodel;
  }

int lqt_get_cmodel(quicktime_t * file, int track)
  {
  if((track < file->total_vtracks) && (track >= 0))
    return file->vtracks[track].io_cmodel;
  else
    return LQT_COLORMODEL_NONE;
  }


void lqt_set_cmodel(quicktime_t *file, int track, int colormodel)
  {
  if((track < file->total_vtracks) && (track >= 0))
    file->vtracks[track].io_cmodel = colormodel;
  else
    fprintf(stderr, "lqt_set_cmodel: No track No. %d\n", track);
  }

void quicktime_set_row_span(quicktime_t *file, int row_span)
{
int i;
  for(i = 0; i < file->total_vtracks; i++)
    file->vtracks[i].io_row_span = row_span;
}

void lqt_set_row_span(quicktime_t *file, int track, int row_span)
  {
  file->vtracks[track].io_row_span = row_span;
  }

void lqt_set_row_span_uv(quicktime_t *file, int track, int row_span_uv)
  {
  file->vtracks[track].io_row_span_uv = row_span_uv;
  }

void quicktime_set_window(quicktime_t *file,
        int in_x,                    /* Location of input frame to take picture */
        int in_y,
        int in_w,
        int in_h,
        int out_w,                   /* Dimensions of output frame */
        int out_h)
{
        if(in_x >= 0 && in_y >= 0 && in_w > 0 && in_h > 0 && out_w > 0 && out_h > 0)
        {
                file->in_x = in_x;
                file->in_y = in_y;
                file->in_w = in_w;
                file->in_h = in_h;
                file->out_w = out_w;
                file->out_h = out_h;
        }
}


void quicktime_set_depth(quicktime_t *file, int depth, int track)
{
	int i;

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.minf.stbl.stsd.table[0].depth = depth;
	}
}

double quicktime_frame_rate(quicktime_t *file, int track)
{
	if(file->total_vtracks > track)
		return (float)file->vtracks[track].track->mdia.mdhd.time_scale / 
			file->vtracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration;

	return 0;
}

/*
 *  Return the timestamp of the NEXT frame to be decoded.
 *  Call this BEFORE one of the decoding functions.
 */
  
int64_t lqt_frame_time(quicktime_t * file, int track)
  {
  return file->vtracks[track].timestamp;
  }

/*
 *  Return the Duration of the entire track
 */

int64_t lqt_video_duration(quicktime_t * file, int track)
  {
  int64_t dummy1;
  int64_t dummy2;
  

  return
    quicktime_sample_to_time(&(file->vtracks[track].track->mdia.minf.stbl.stts), -1,
                             &dummy1, &dummy2);
  }


/*
 *  Get the timescale of the track. Divide the return values
 *  of lqt_frame_duration and lqt_frame_time by the scale to
 *  get the time in seconds.
 */
  
int lqt_video_time_scale(quicktime_t * file, int track)
  {
  if(file->total_vtracks <= track)
    return 0;
  return file->vtracks[track].track->mdia.mdhd.time_scale;
  }

/*
 *  Get the duration of the NEXT frame to be decoded.
 *  If constant is not NULL it will be set to 1 if the
 *  frame duration is constant throughout the whole track
 */

int lqt_frame_duration(quicktime_t * file, int track, int *constant)
  {
  if(file->total_vtracks <= track)
    return 0;

  if(constant)
    {
    if(file->vtracks[track].track->mdia.minf.stbl.stts.total_entries == 1)
      *constant = 1;
    else if((file->vtracks[track].track->mdia.minf.stbl.stts.total_entries == 2) && 
            (file->vtracks[track].track->mdia.minf.stbl.stts.table[1].sample_count == 1))
      *constant = 1;
    else
      *constant = 0;
    }
  return
    file->vtracks[track].track->mdia.minf.stbl.stts.table[file->vtracks[track].stts_index].sample_duration;
  }


char* quicktime_video_compressor(quicktime_t *file, int track)
{
	return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int quicktime_write_audio(quicktime_t *file, 
	uint8_t *audio_buffer, 
	long samples, 
	int track)
{
        int result;
        int64_t bytes;
        quicktime_atom_t chunk_atom;
        quicktime_audio_map_t *track_map = &file->atracks[track];
        quicktime_trak_t *trak = track_map->track;
                                                                                                                  
/* write chunk for 1 track */
        bytes = samples * quicktime_audio_bits(file, track) / 8 * file->atracks[track].channels;
        quicktime_write_chunk_header(file, trak, &chunk_atom);
        result = !quicktime_write_data(file, audio_buffer, bytes);
        quicktime_write_chunk_footer(file,
                                        trak,
                                        track_map->current_chunk,
                                        &chunk_atom,
                                        samples);
                                                                                                                  
/*      file->atracks[track].current_position += samples; */
        file->atracks[track].current_chunk++;
        return result;
}

int quicktime_write_frame(quicktime_t *file, unsigned char *video_buffer, int64_t bytes, int track)
{
        int result = 0;
        quicktime_atom_t chunk_atom;
        quicktime_video_map_t *vtrack = &file->vtracks[track];
        quicktime_trak_t *trak = vtrack->track;
                                                                                                                  
        quicktime_write_chunk_header(file, trak, &chunk_atom);
        result = !quicktime_write_data(file, video_buffer, bytes);
        quicktime_write_chunk_footer(file,
                                        trak,
                                        vtrack->current_chunk,
                                        &chunk_atom,
                                        1);
        file->vtracks[track].current_position++;
        file->vtracks[track].current_chunk++;
        return result;
}

long quicktime_frame_size(quicktime_t *file, long frame, int track)
{
	long bytes = 0;
	quicktime_trak_t *trak = file->vtracks[track].track;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		bytes = trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	{
		long total_frames = quicktime_track_samples(file, trak);
		if(frame < 0) frame = 0;
		else
		if(frame > total_frames - 1) frame = total_frames - 1;
		bytes = trak->mdia.minf.stbl.stsz.table[frame].size;
	}


	return bytes;
}


long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track)
{
	int64_t bytes;
	int result = 0;

	bytes = quicktime_frame_size(file, file->vtracks[track].current_position, track);

	quicktime_set_video_position(file, file->vtracks[track].current_position, track);
	result = quicktime_read_data(file, video_buffer, bytes);
        lqt_update_frame_position(&file->vtracks[track]);

        if(!result) return 0;
	return bytes;
}

long quicktime_get_keyframe_before(quicktime_t *file, long frame, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	int i;





// Offset 1
	frame++;


	for(i = stss->total_entries - 1; i >= 0; i--)
	{
		if(stss->table[i].sample <= frame) return stss->table[i].sample - 1;
	}

	return 0;
}

long quicktime_get_keyframe_after(quicktime_t *file, long frame, int track)
{
        quicktime_trak_t *trak = file->vtracks[track].track;
        quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
        int i;
                                                                                                                  
                                                                                                                  
                                                                                                                  
                                                                                                                  
                                                                                                                  
// Offset 1
        frame++;
                                                                                                                  
                                                                                                                  
        for(i = 0; i < stss->total_entries; i++)
        {
                if(stss->table[i].sample >= frame) return stss->table[i].sample - 1;
        }
                                                                                                                  
        return 0;
}


void quicktime_insert_keyframe(quicktime_t *file, long frame, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	int i;

// Set keyframe flag in idx1 table.
// Only possible in the first RIFF.  After that, there's no keyframe support.
        if(file->use_avi && file->total_riffs == 1)
                quicktime_set_idx1_keyframe(file,
                        trak,
                        frame);


// Offset 1
	frame++;


//printf("quicktime_insert_keyframe 1\n");
// Get the keyframe greater or equal to new frame
	for(i = 0; i < stss->total_entries; i++)
	{
		if(stss->table[i].sample >= frame) break;
	}

// Expand table
	if(stss->entries_allocated <= stss->total_entries)
	{
		stss->entries_allocated *= 2;
		stss->table = realloc(stss->table, sizeof(quicktime_stss_table_t) * stss->entries_allocated);
	}

// Insert before existing frame
	if(i < stss->total_entries)
	{
		if(stss->table[i].sample > frame)
		{
			int j, k;
			for(j = stss->total_entries, k = stss->total_entries - 1;
				k >= i;
				j--, k--)
			{
				stss->table[j] = stss->table[k];
			}
			stss->table[i].sample = frame;
		}
	}
	else
// Insert after last frame
		stss->table[i].sample = frame;

	stss->total_entries++;
//printf("quicktime_insert_keyframe 2 %d\n", stss->total_entries);
}


int quicktime_has_keyframes(quicktime_t *file, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	
	return stss->total_entries > 0;
}





int quicktime_read_frame_init(quicktime_t *file, int track)
{
	quicktime_set_video_position(file, file->vtracks[track].current_position, track);
	if(quicktime_ftell(file) != file->file_position) 
	{
		fseeko(file->stream, file->file_position, SEEK_SET);
		file->ftell_position = file->file_position;
	}
	return 0;
}

int quicktime_read_frame_end(quicktime_t *file, int track)
{
	file->file_position = quicktime_ftell(file);
	file->vtracks[track].current_position++;
	return 0;
}

int quicktime_init_video_map(quicktime_video_map_t *vtrack,
                             quicktime_trak_t *trak,
                             int encode,
                             lqt_codec_info_t * info)
{
	vtrack->track = trak;
	vtrack->current_position = 0;
	vtrack->current_chunk = 1;
        vtrack->io_cmodel = BC_RGB888;
	quicktime_init_vcodec(vtrack, encode, info);
	return 0;
}

int quicktime_delete_video_map(quicktime_video_map_t *vtrack)
{
	quicktime_delete_vcodec(vtrack);
        if(vtrack->temp_frame)
          lqt_rows_free(vtrack->temp_frame);
	return 0;
}

int quicktime_init_audio_map(quicktime_audio_map_t *atrack, quicktime_trak_t *trak, int encode, lqt_codec_info_t * info)
{
	atrack->track = trak;
	atrack->channels = trak->mdia.minf.stbl.stsd.table[0].channels;
	atrack->current_position = 0;
	atrack->current_chunk = 1;
	quicktime_init_acodec(atrack, encode, info);
	return 0;
}

int quicktime_delete_audio_map(quicktime_audio_map_t *atrack)
{
	quicktime_delete_acodec(atrack);
        if(atrack->sample_buffer)
          free(atrack->sample_buffer);
        return 0;
}

void quicktime_init_maps(quicktime_t * file)
  {
  int i, dom, track;
  /* get tables for all the different tracks */
  file->total_atracks = quicktime_audio_tracks(file);

  if(file->total_atracks)
    {
    file->atracks = (quicktime_audio_map_t*)calloc(1, sizeof(quicktime_audio_map_t) * file->total_atracks);
    for(i = 0, track = 0; i < file->total_atracks; i++, track++)
      {
      while(!file->moov.trak[track]->mdia.minf.is_audio)
        track++;
      quicktime_init_audio_map(&(file->atracks[i]), file->moov.trak[track],
                               file->wr,
                               (lqt_codec_info_t*)0);
      }
    }

  file->total_vtracks = quicktime_video_tracks(file);

  if(file->total_vtracks)
    {
    file->vtracks = (quicktime_video_map_t*)calloc(1, sizeof(quicktime_video_map_t) * file->total_vtracks);
    
    for(track = 0, i = 0; i < file->total_vtracks; i++, track++)
      {
      while(!file->moov.trak[track]->mdia.minf.is_video)
        track++;
      
      quicktime_init_video_map(&(file->vtracks[i]), file->moov.trak[track],
                               file->wr,
                               (lqt_codec_info_t*)0);
      /* Get decoder colormodel */
      ((quicktime_codec_t*)file->vtracks[i].codec)->decode_video(file, (uint8_t**)0, i);
      file->vtracks[i].io_cmodel = file->vtracks[i].stream_cmodel;
      
      lqt_get_default_rowspan(file->vtracks[i].io_cmodel,
                              quicktime_video_width(file, i),
                              &(file->vtracks[i].io_row_span),
                              &(file->vtracks[i].io_row_span_uv));
      file->vtracks[i].stream_row_span = file->vtracks[i].io_row_span;
      file->vtracks[i].stream_row_span_uv = file->vtracks[i].io_row_span_uv;

      /* Get interlace mode */
      if(file->vtracks[i].interlace_mode == LQT_INTERLACE_NONE)
        {
        dom = file->vtracks[i].track->mdia.minf.stbl.stsd.table[0].field_dominance;
        if (file->vtracks[i].track->mdia.minf.stbl.stsd.table[0].fields == 2)
          {
          if (dom == 14 || dom == 6)
            file->vtracks[i].interlace_mode = LQT_INTERLACE_BOTTOM_FIRST;
          else if (dom == 9 || dom == 1)
            file->vtracks[i].interlace_mode = LQT_INTERLACE_TOP_FIRST;
          }
        }
      }
    }
  }

int quicktime_read_info(quicktime_t *file)
{
        int result = 0, got_header = 0;
        int64_t start_position = quicktime_position(file);
        quicktime_atom_t leaf_atom;
        uint8_t avi_avi[4];
        int got_avi = 0;
        int got_asf = 0;
                                                                                                                  
        quicktime_set_position(file, 0LL);
                                                                                                                  
/* Test file format */
        do
        {
                file->use_avi = 1;
                file->use_asf = 1;
                result = quicktime_atom_read_header(file, &leaf_atom);
                if(!result && quicktime_atom_is(&leaf_atom, "RIFF"))
                {
                        quicktime_read_data(file, avi_avi, 4);
                        if(quicktime_match_32(avi_avi, "AVI "))
                        {
                                got_avi = 1;
                        }
                        else
                        {
                                result = 0;
                                break;
                        }
                }
                else
                {
                        result = 0;
                        break;
                }
        }while(1);
        if(!got_avi) file->use_avi = 0;
        if(!got_asf) file->use_asf = 0;
                                                                                                                  
        quicktime_set_position(file, 0LL);
                                                                                                                  
/* McRoweSoft AVI section */
        if(file->use_avi)
        {
//printf("quicktime_read_info 1\n");
/* Import first RIFF */
                do
                {
                        result = quicktime_atom_read_header(file, &leaf_atom);
                        if(!result)
                        {
                                if(quicktime_atom_is(&leaf_atom, "RIFF"))
                                {
                                        quicktime_read_riff(file, &leaf_atom);
/* Return success */
                                        got_header = 1;
                                }
                        }
                }while(!result &&
                        !got_header &&
                        quicktime_position(file) < file->total_length);
                                                                                                                  
//printf("quicktime_read_info 10\n");
/* Construct indexes. */
                if(quicktime_import_avi(file))
                  return 1;
                //printf("quicktime_read_info 20\n");
        }
/* Quicktime section */
        else
        if(!file->use_avi)
        {
                do
                {
                        result = quicktime_atom_read_header(file, &leaf_atom);
                                                                                                                  
                        if(!result)
                        {
                                if(quicktime_atom_is(&leaf_atom, "mdat"))
                                {
                                        quicktime_read_mdat(file, &(file->mdat), &leaf_atom);
                                }
                                else
                                if(quicktime_atom_is(&leaf_atom, "moov"))
                                {
/* Set preload and preload the moov atom here */
                                        int64_t start_position = quicktime_position(file);
                                        long temp_size = leaf_atom.end - start_position;
                                        unsigned char *temp = malloc(temp_size);
                                        quicktime_set_preload(file,
                                                (temp_size < 0x100000) ? 0x100000 : temp_size);
                                        quicktime_read_data(file, temp, temp_size);
                                        quicktime_set_position(file, start_position);
                                        free(temp);
                                                                                                                  
                                        quicktime_read_moov(file, &(file->moov), &leaf_atom);
                                        got_header = 1;
                                }
                                else
                                        quicktime_atom_skip(file, &leaf_atom);
                        }
                }while(!result && quicktime_position(file) < file->total_length);
		
		/* read QTVR sample atoms */
		if (lqt_qtvr_get_object_track(file) >= 0)
		{
			quicktime_qtatom_t leaf_atom, root_atom;
			int64_t start_position = quicktime_position(file);
			quicktime_set_position(file, file->moov.trak[lqt_qtvr_get_object_track(file)]->mdia.minf.stbl.stco.table[0].offset);
			quicktime_qtatom_read_container_header(file);
			/* root qtatom "sean" */
			quicktime_qtatom_read_header(file, &root_atom);
			
			do
			{
				quicktime_qtatom_read_header(file, &leaf_atom);
				if(quicktime_qtatom_is(&leaf_atom, "obji"))
				{
					quicktime_read_obji(file, &file->qtvr_node[0].obji);					
				}     
				else
				if(quicktime_qtatom_is(&leaf_atom, "ndhd"))
				{
					quicktime_read_ndhd(file, &file->qtvr_node[0].ndhd);					
				}     
				else
                                        quicktime_qtatom_skip(file, &leaf_atom);
			} while(quicktime_position(file) < root_atom.end);
			
			quicktime_set_position(file, start_position);
		}

		if (lqt_qtvr_get_qtvr_track(file) >= 0)
		{
			quicktime_qtatom_t leaf_atom, root_atom;
			int64_t start_position = quicktime_position(file);
			quicktime_set_position(file, file->moov.trak[lqt_qtvr_get_qtvr_track(file)]->mdia.minf.stbl.stco.table[0].offset);
			quicktime_qtatom_read_container_header(file);
			/* root qtatom "sean" */
			quicktime_qtatom_read_header(file, &root_atom);
			
			do
			{
				quicktime_qtatom_read_header(file, &leaf_atom);
				if(quicktime_qtatom_is(&leaf_atom, "ndhd"))
				{
					quicktime_read_ndhd(file, &file->qtvr_node[0].ndhd);					
				}     
				else
                                        quicktime_qtatom_skip(file, &leaf_atom);
			} while(quicktime_position(file) < root_atom.end);
			
			quicktime_set_position(file, start_position);
		}
/* go back to the original position */
                quicktime_set_position(file, start_position);
                                                                                                                  
        }
                                                                                                                  
/* Initialize track map objects */
        if(got_header)
        {
                quicktime_init_maps(file);
        }
                                                                                                                  
/* Shut down preload in case of an obsurdly high temp_size */
        quicktime_set_preload(file, 0);
                                                                                                                  
//printf("quicktime_read_info 100\n");
        return !got_header;
}


int quicktime_dump(quicktime_t *file)
{
	printf("quicktime_dump\n");
	printf("movie data\n");
	printf(" size %lld\n", file->mdat.atom.size);
	printf(" start %lld\n", file->mdat.atom.start);
	quicktime_moov_dump(&(file->moov));
	if (lqt_qtvr_get_object_track(file) >= 0)
	{
		quicktime_obji_dump(&(file->qtvr_node[0].obji));
	}
	if (lqt_qtvr_get_qtvr_track(file) >= 0)
	{
		quicktime_ndhd_dump(&(file->qtvr_node[0].ndhd));
	}
	return 0;
}


// ================================== Entry points =============================

int quicktime_check_sig(char *path)
{
        quicktime_t file;
        quicktime_atom_t leaf_atom;
        int result = 0, result1 = 0, result2 = 0;
        uint8_t avi_test[12];
                                                                                                                  
        quicktime_init(&file);
        result = quicktime_file_open(&file, path, 1, 0);
                                                                                                                  
        if(!result)
        {
// Check for Microsoft AVI
                quicktime_read_data(&file, avi_test, 12);
                quicktime_set_position(&file, 0);
                if(quicktime_match_32(avi_test, "RIFF") &&
                        quicktime_match_32(avi_test + 8, "AVI "))
                {
                        result2 = 1;
                }
                else
                {
                        do
                        {
                                result1 = quicktime_atom_read_header(&file, &leaf_atom);
                                                                                                                  
                                if(!result1)
                                {
/* just want the "moov" atom */
                                        if(quicktime_atom_is(&leaf_atom, "moov"))
                                        {
                                                result2 = 1;
                                        }
                                        else
                                                quicktime_atom_skip(&file, &leaf_atom);
                                }
                        }while(!result1 && !result2 && quicktime_position(&file) < file.total_length);
                }
        }
                                                                                                                  
//printf(__FUNCTION__ " 2 %d\n", result2);
        quicktime_file_close(&file);
        quicktime_delete(&file);
        return result2;
}

void quicktime_set_avi(quicktime_t *file, int value)
{
        file->use_avi = value;
        quicktime_set_position(file, 0);
                                                                                                                  
// Write RIFF chunk
        quicktime_init_riff(file);
}
                                                                                                                  
int quicktime_is_avi(quicktime_t *file)
{
        return file->use_avi;
}
                                                                                                                  
                                                                                                                  
void quicktime_set_asf(quicktime_t *file, int value)
{
        file->use_asf = value;
}


quicktime_t* quicktime_open(const char *filename, int rd, int wr)
{
        int i;
	quicktime_t *new_file;
	int result = 0;

        new_file = calloc(1, sizeof(*new_file));

        if(rd && wr)
          {
	  fprintf(stderr, "read/write mode is not supported\n");
          free(new_file);
          return (quicktime_t*)0;
          }
	
#ifdef PRINT_BANNER
	static int have_warned = 0;

	if( ! have_warned )
	{
		printf( "WARNING: This program is using a beta version of libquicktime.\n" );
		have_warned = 1;
	}
#endif

	quicktime_init(new_file);
	new_file->wr = wr;
	new_file->rd = rd;
	new_file->mdat.atom.start = 0;

        result = quicktime_file_open(new_file, filename, rd, wr);

        if(!result)
          {
          if(rd)
            {
            if(quicktime_read_info(new_file))
              {
              quicktime_close(new_file);
              fprintf(stderr, "quicktime_open: error in header\n");
              new_file = 0;
              }
            //printf("quicktime_open 3\n");
            }
          
          //printf("quicktime_open 4 %d %d\n", wr, exists);
          /* start the data atom */
          /* also don't want to do this if making a streamable file */
          if(wr)
            {
            quicktime_atom_write_header64(new_file, 
                                          &new_file->mdat.atom, 
                                          "mdat");
            }
          }
        else
          {
          quicktime_close(new_file);
          new_file = 0;
          }
        
        //printf("quicktime_open 5 %llx %llx\n", new_file->ftell_position, new_file->file_position);
        
        
	if(rd && new_file)
          {
		/* Set default decoding parameters */
          for(i = 0; i < new_file->total_atracks; i++)
            lqt_set_default_audio_parameters(new_file, i);

          for(i = 0; i < new_file->total_vtracks; i++)
            lqt_set_default_video_parameters(new_file, i);
          }
        
	return new_file;
}

int quicktime_close(quicktime_t *file)
{
        int result = 0;
        if(file->wr)
        {
                quicktime_codecs_flush(file);
                                                                                                                  
                if(file->use_avi)
                {
#if 0
                        quicktime_atom_t junk_atom;
                        int i;
                        int64_t position = quicktime_position(file);
#endif
// Finalize last header
                        quicktime_finalize_riff(file, file->riff[file->total_riffs - 1]);
                                                                                                                  
                                                                                                                  
// Finalize the odml header
                        quicktime_finalize_odml(file, &file->riff[0]->hdrl);
                                                                                                                  
// Finalize super indexes
                        if(file->total_riffs > 1)
                          quicktime_finalize_indx(file);
                        
#if 0
// Pad ending
                        quicktime_set_position(file, position);
                        quicktime_atom_write_header(file, &junk_atom, "JUNK");
                        for(i = 0; i < 0x406; i++)
                                quicktime_write_int32_le(file, 0);
                        quicktime_atom_write_footer(file, &junk_atom);
#endif
                }
                else
                {
		    	if (lqt_qtvr_get_object_track(file) >= 0)
			{
			    lqt_qtvr_add_node(file);
			    lqt_qtvr_add_node(file);
			}
// Atoms are only written here
                        quicktime_finalize_moov(file, &(file->moov));
                        quicktime_write_moov(file, &(file->moov));
                        quicktime_atom_write_footer(file, &file->mdat.atom);
                }
        }
                                                                                                                  
        quicktime_file_close(file);
                                                                                                                  
        quicktime_delete(file);
        free(file);
        return result;
}



/*
 *  Apply default parameters for a codec
 */

static void apply_default_parameters(quicktime_t * file,
                                     int track,
                                     quicktime_codec_t * codec,
                                     lqt_codec_info_t * codec_info,
                                     int encode)
  {
  int num_parameters;
  lqt_parameter_info_t * parameter_info;
  int j;

  //  fprintf(stderr, "APPLY CODEC INFO %s %d\n", codec_info->name, encode);
  
  if(encode)
    {
    num_parameters = codec_info->num_encoding_parameters;
    parameter_info = codec_info->encoding_parameters;
    }
  else
    {
    num_parameters = codec_info->num_decoding_parameters;
    parameter_info = codec_info->decoding_parameters;
    }
  
  for(j = 0; j < num_parameters; j++)
    {
    switch(parameter_info[j].type)
      {
      case LQT_PARAMETER_INT:
#ifndef NDEBUG
        fprintf(stderr, "Setting Parameter %s to %d\n",
                parameter_info[j].name,
                parameter_info[j].val_default.val_int);
#endif
        codec->set_parameter(file, track, parameter_info[j].name,
                             &(parameter_info[j].val_default.val_int));
        break;
      case LQT_PARAMETER_STRING:
      case LQT_PARAMETER_STRINGLIST:
#ifndef NDEBUG
        fprintf(stderr, "Setting Parameter %s to %s\n",
                parameter_info[j].name,
                parameter_info[j].val_default.val_string);
#endif
        codec->set_parameter(file, track, parameter_info[j].name,
                             &(parameter_info[j].val_default.val_string));
        break;
      case LQT_PARAMETER_SECTION:
        break; /* NOP */
      }

    }                      
  }

void lqt_set_default_video_parameters(quicktime_t * file, int track)
  {
  lqt_codec_info_t ** codec_info;
  quicktime_codec_t * codec;
    
  codec = (quicktime_codec_t*)(file->vtracks[track].codec);
  codec_info = lqt_find_video_codec_by_name(codec->codec_name);
  if(codec_info)
    {
    apply_default_parameters(file, track, codec, *codec_info, file->wr);
    lqt_destroy_codec_info(codec_info);
    }
  }

int lqt_set_fiel(quicktime_t *file, int track, int nfields, int dominance)
	{
	quicktime_stsd_table_t *stsdt_p;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;

	if	(nfields !=1 && nfields != 2)
		return 0;

/*
 * http://developer.apple.com/quicktime/icefloe/dispatch019.html#fiel
 *
 * "dominance" is what Apple calls "detail".  From what I can figure out
 * the term "bottom field first" corresponds to a "detail" setting of 14 and
 * "top field first" is a "detail" setting of 9.
*/
	switch	(dominance)
		{
		case	0:
		case	1:
		case	6:
		case	9:
		case	14:
			break;
		default:
			return 0;
		}

	stsdt_p = file->vtracks[track].track->mdia.minf.stbl.stsd.table;
	stsdt_p->fields = nfields;
	stsdt_p->field_dominance = dominance;
	return 1;
	}
	
int lqt_get_fiel(quicktime_t *file, int track, int *nfields, int *dominance)
	{
	quicktime_stsd_table_t *stsdt_p;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;

	stsdt_p = file->vtracks[track].track->mdia.minf.stbl.stsd.table;

	if	(nfields != NULL)
		*nfields = stsdt_p->fields;
	
	if	(dominance != NULL)
		*dominance = stsdt_p->field_dominance;
	return 1;
	}

void lqt_set_default_audio_parameters(quicktime_t * file, int track)
  {
  int i;
  lqt_codec_info_t ** codec_info;
  quicktime_codec_t * codec;
  for(i = 0; i < file->total_atracks; i++)
    {
    codec = (quicktime_codec_t*)(file->atracks[i].codec);
    codec_info = lqt_find_audio_codec_by_name(codec->codec_name);
    if(codec_info)
      {
      apply_default_parameters(file, i, codec, *codec_info, file->wr);
      lqt_destroy_codec_info(codec_info);
      }
    }
  }

int quicktime_major()
{
        return QUICKTIME_MAJOR;
}

int quicktime_minor()
{
        return QUICKTIME_MINOR;
}

int quicktime_release()
{
        return QUICKTIME_RELEASE;
}

int quicktime_div3_is_key(unsigned char *data, long size)
  {
  int result = 0;

// First 2 bits are pict type.
  result = (data[0] & 0xc0) == 0;


  return result;
  }


/*
 *  AVI Specific stuff
 */

int lqt_is_avi(quicktime_t *file)
  {
  return !!file->use_avi;
  }

int lqt_get_wav_id(quicktime_t *file, int track)
  {
  quicktime_trak_t * trak;
  trak = file->atracks[track].track;
  return trak->mdia.minf.stbl.stsd.table[0].compression_id;
  }

int64_t * lqt_get_chunk_sizes(quicktime_t * file, quicktime_trak_t *trak)
  {
  int i, j;
  int64_t * ret;
  int64_t next_offset;
  long num_chunks;
  int num_tracks;
  int * chunk_indices;
  
  num_chunks = trak->mdia.minf.stbl.stco.total_entries;
  ret = calloc(num_chunks, sizeof(int64_t));

  num_tracks = file->moov.total_tracks;

  chunk_indices = malloc(num_tracks * sizeof(int));

  for(i = 0; i < num_tracks; i++)
    {
    chunk_indices[i] = 0;
    }
  
  for(i = 0; i < num_chunks; i++)
    {
    next_offset = -1;
    for(j = 0; j < num_tracks; j++)
      {
      if(chunk_indices[j] < 0)
        continue;

      while(file->moov.trak[j]->mdia.minf.stbl.stco.table[chunk_indices[j]].offset <=
            trak->mdia.minf.stbl.stco.table[i].offset)
        {
        if(chunk_indices[j] >= file->moov.trak[j]->mdia.minf.stbl.stco.total_entries - 1)
          {
          chunk_indices[j] = -1;
          break;
          }
        else
          chunk_indices[j]++;
        }
      if(chunk_indices[j] < 0)
        continue;
      if((next_offset == -1) ||
         (file->moov.trak[j]->mdia.minf.stbl.stco.table[chunk_indices[j]].offset < next_offset))
        next_offset = file->moov.trak[j]->mdia.minf.stbl.stco.table[chunk_indices[j]].offset;
      }
    if(next_offset > 0)
      {
      ret[i] = next_offset - trak->mdia.minf.stbl.stco.table[i].offset;

      if(file->use_avi)
        ret[i] -= 8;
      
      }
    else /* Last chunk: Take the end of the mdat atom */
      {
      ret[i] = file->mdat.atom.start +  file->mdat.atom.size - trak->mdia.minf.stbl.stco.table[i].offset;
      if(ret[i] < 0)
        ret[i] = 0;
#if 0
      fprintf(stderr, "Chunk size %lld, file->mdat.atom.start: %lld file->mdat.atom.size: %lld, trak->mdia.minf.stbl.stco.table[i].offset: %lld\n",
              ret[i], file->mdat.atom.start, file->mdat.atom.size,
              trak->mdia.minf.stbl.stco.table[i].offset);
#endif
      }
    }
  free(chunk_indices);
  return ret;
  }

int lqt_read_audio_chunk(quicktime_t * file, int track,
                         long chunk,
                         uint8_t ** buffer, int * buffer_alloc, int * samples)
  {
  int64_t offset;
  quicktime_trak_t * trak;
  int result;

  trak = file->atracks[track].track;

  if(chunk > trak->mdia.minf.stbl.stco.total_entries)
    {
    /* Read beyond EOF */
    file->atracks[track].eof = 1;
    return 0;
    }
#if 0
  fprintf(stderr, "lqt_read_audio_chunk %d %d\n",
          chunk,
          file->atracks[track].track->mdia.minf.stbl.stco.total_entries);
#endif
  if(!trak->chunk_sizes)
    {
    trak->chunk_sizes = lqt_get_chunk_sizes(file, trak);
    }
  if(samples)
    *samples = quicktime_chunk_samples(trak, chunk);
  /* Reallocate buffer */

  if(*buffer_alloc < trak->chunk_sizes[chunk-1] + 16)
    {
    *buffer_alloc = trak->chunk_sizes[chunk-1] + 32;
    *buffer = realloc(*buffer, *buffer_alloc);
    }
  
  /* Get offset */
  
  offset = quicktime_chunk_to_offset(file, trak, chunk);

  quicktime_set_position(file, offset);

  result = quicktime_read_data(file, *buffer, trak->chunk_sizes[chunk-1]);

  memset((*buffer) + trak->chunk_sizes[chunk-1], 0, 16);
  
  return result ? trak->chunk_sizes[chunk-1] : 0;
  }

int lqt_append_audio_chunk(quicktime_t * file, int track,
                           long chunk,
                           uint8_t ** buffer, int * buffer_alloc,
                           int initial_bytes)
  {
  int64_t offset;
  quicktime_trak_t * trak;
  int result;

  trak = file->atracks[track].track;

  if(chunk > trak->mdia.minf.stbl.stco.total_entries)
    {
    /* Read beyond EOF */
    file->atracks[track].eof = 1;
    return 0;
    }

  if(!trak->chunk_sizes)
    {
    trak->chunk_sizes = lqt_get_chunk_sizes(file, trak);
    }

  /* Reallocate buffer */

  if(*buffer_alloc < trak->chunk_sizes[chunk-1] + 16 + initial_bytes)
    {
    *buffer_alloc = trak->chunk_sizes[chunk-1] + 32 + initial_bytes;
    *buffer = realloc(*buffer, *buffer_alloc);
    }
  
  /* Get offset */
  
  offset = quicktime_chunk_to_offset(file, trak, chunk);

  quicktime_set_position(file, offset);

  result = quicktime_read_data(file, (*buffer) + initial_bytes, trak->chunk_sizes[chunk-1]);

  memset((*buffer) + initial_bytes + trak->chunk_sizes[chunk-1], 0, 16);
  
  return result ? trak->chunk_sizes[chunk-1] : 0;
  }

int lqt_read_video_frame(quicktime_t * file, int track,
                         long frame,
                         uint8_t ** buffer, int * buffer_alloc)
  {
  int size;
  quicktime_set_video_position(file, frame, track);
  size = quicktime_frame_size(file,  frame, track);

  if(*buffer_alloc < size)
    {
    *buffer_alloc = size + 1024;
    *buffer = realloc(*buffer, *buffer_alloc);
    }
  
  return quicktime_read_data(file, *buffer, size);
  }

int64_t lqt_last_audio_position(quicktime_t * file, int track)
  {
  return file->atracks[track].last_position;
  }

/* Interlace mode */

static struct
  {
  lqt_interlace_mode_t mode;
  const char * name;
  }
interlace_modes[] =
  {
    { LQT_INTERLACE_NONE,         "None (Progressive)" },
    { LQT_INTERLACE_TOP_FIRST,    "Top field first"    },
    { LQT_INTERLACE_BOTTOM_FIRST, "Bottom field first" },
    {  /* End of array */ }
  };
  
lqt_interlace_mode_t lqt_get_interlace_mode(quicktime_t * file, int track)
  {
  if(track < 0 || track > file->total_vtracks)
    return LQT_INTERLACE_NONE;
  return file->vtracks[track].interlace_mode;
  }

const char * lqt_interlace_mode_to_string(lqt_interlace_mode_t mode)
  {
  int i = 0;
  while(interlace_modes[i].name)
    {
    if(interlace_modes[i].mode == mode)
      return interlace_modes[i].name;
    i++;
    }
  return (const char*)0;
  }

/* Chroma placement */

static struct
  {
  lqt_chroma_placement_t placement;
  const char * name;
  }
chroma_placements[] =
  {
    { LQT_CHROMA_PLACEMENT_DEFAULT,  "MPEG-1/JPEG" },
    { LQT_CHROMA_PLACEMENT_MPEG2,    "MPEG-1" },
    { LQT_CHROMA_PLACEMENT_DVPAL,    "PAL DV" },
    {  /* End of array */ }
  };

const char * lqt_chroma_placement_to_string(lqt_chroma_placement_t p)
  {
  int i = 0;
  while(chroma_placements[i].name)
    {
    if(chroma_placements[i].placement == p)
      return chroma_placements[i].name;
    i++;
    }
  return (const char*)0;
  }

lqt_chroma_placement_t lqt_get_chroma_placement(quicktime_t * file, int track)
  {
  if(track < 0 || track > file->total_vtracks)
    return LQT_INTERLACE_NONE;
  return file->vtracks[track].chroma_placement;
  }

/* Sample format */

static struct
  {
  lqt_sample_format_t format;
  const char * name;
  }
sample_formats[] =
  {
    { LQT_SAMPLE_UNDEFINED, "Undefined" }, /* If this is returned, we have an error */
    { LQT_SAMPLE_INT8, "8 bit signed" },
    { LQT_SAMPLE_UINT8, "8 bit unsigned" },
    { LQT_SAMPLE_INT16, "16 bit signed" },
    { LQT_SAMPLE_INT32, "32 bit signed" },
    { LQT_SAMPLE_FLOAT, "Floating point" }, /* Float is ALWAYS machine native */
    {  /* End of array */ }
  };

const char * lqt_sample_format_to_string(lqt_sample_format_t s)
  {
  int i = 0;
  while(sample_formats[i].name)
    {
    if(sample_formats[i].format == s)
      return sample_formats[i].name;
    i++;
    }
  return (const char*)0;
  }

lqt_sample_format_t lqt_get_sample_format(quicktime_t * file, int track)
  {
  if(track < 0 || track > file->total_atracks)
    return LQT_SAMPLE_UNDEFINED;
  return file->atracks[track].sample_format;
  }

void lqt_init_vbr_audio(quicktime_t * file, int track)
  {
  quicktime_trak_t * trak = file->atracks[track].track;
  trak->mdia.minf.stbl.stsd.table[0].compression_id = -2;
  trak->mdia.minf.is_audio_vbr = 1;
  }

void lqt_start_audio_vbr_chunk(quicktime_t * file, int track)
  {
  file->atracks[track].vbr_num_frames = 0;
  }

void lqt_start_audio_vbr_frame(quicktime_t * file, int track)
  {
  quicktime_audio_map_t * atrack = &file->atracks[track];
  atrack->vbr_frame_start = quicktime_position(file);
  //  fprintf(stderr, "lqt_start_audio_vbr_frame\n");
  }

void lqt_finish_audio_vbr_frame(quicktime_t * file, int track, int num_samples)
  {
  quicktime_stsz_t * stsz;
  quicktime_stts_t * stts;
  quicktime_audio_map_t * atrack = &file->atracks[track];
  
  stsz = &(file->atracks[track].track->mdia.minf.stbl.stsz);
  stts = &(file->atracks[track].track->mdia.minf.stbl.stts);
  
  /* Update stsz */

  quicktime_update_stsz(stsz, file->atracks[track].vbr_frames_written, 
                        quicktime_position(file) - file->atracks[track].vbr_frame_start);
  /* Update stts */
  
  quicktime_update_stts(stts, file->atracks[track].vbr_frames_written, num_samples);

  atrack->vbr_num_frames++;
  atrack->vbr_frames_written++;

  //  fprintf(stderr, "lqt_finish_audio_vbr_frame (total_frames: %lld, samples: %d)\n",
  //          atrack->vbr_frames_written, num_samples);
  
  }

/* VBR Reading support */

/* Check if VBR reading should be enabled */

int lqt_audio_is_vbr(quicktime_t * file, int track)
  {
  return file->atracks[track].track->mdia.minf.is_audio_vbr;
  }

/*
 *  Helper function: Get the "durarion of a sample range" (which means the
 *  uncompressed samples in a range of VBR packets)
 */

static int get_uncompressed_samples(quicktime_stts_t * stts, long start_sample, long end_sample)
  {
  long count, i, stts_index, stts_count, ret;

  count = 0;
  ret = 0;
  
  for(i = 0; i < stts->total_entries; i++)
    {
    if(count + stts->table[i].sample_count > start_sample)
      {
      stts_index = i;
      stts_count = start_sample - count;
      break;
      }
    count += stts->table[i].sample_count;
    }

  ret = 0;
  for(i = start_sample; i < end_sample; i++)
    {
    ret += stts->table[stts_index].sample_duration;
    stts_count++;
    if(stts_count >= stts->table[stts_index].sample_count)
      {
      stts_index++;
      stts_count = 0;
      }
    }
  return ret;
  }

/* Determine the number of VBR packets (=samples) in one chunk */

int lqt_audio_num_vbr_packets(quicktime_t * file, int track, long chunk, int * samples)
  {
  int64_t start_sample;
  
  quicktime_trak_t * trak;
  long result;
  quicktime_stsc_t *stsc;
  long i;

  
  trak = file->atracks[track].track;
    
  stsc = &(trak->mdia.minf.stbl.stsc);

  if(chunk >= trak->mdia.minf.stbl.stco.total_entries)
    return 0;
  
  i = stsc->total_entries - 1;
  
  if(!stsc->total_entries)
    return 0;
  
  start_sample = 0;

  for(i = 0; i < stsc->total_entries; i++)
    {
    if(((i < stsc->total_entries - 1) && (stsc->table[i+1].chunk > chunk)) ||
       (i == stsc->total_entries - 1))
      {
      start_sample += (chunk - stsc->table[i].chunk) * stsc->table[i].samples;
      result = stsc->table[i].samples;
      break;
      }
    else
      start_sample += (stsc->table[i+1].chunk - stsc->table[i].chunk) * stsc->table[i].samples;
    }
  if(samples)
    *samples = get_uncompressed_samples(&(trak->mdia.minf.stbl.stts), start_sample, start_sample + result);
  
  return result;
  }

/* Read one VBR packet */
int lqt_audio_read_vbr_packet(quicktime_t * file, int track, long chunk, int packet,
                              uint8_t ** buffer, int * buffer_alloc, int * samples)
  {
  int64_t offset;
  long i, stsc_index;
  quicktime_trak_t * trak;
  quicktime_stsc_t *stsc;
  int packet_size;
  long first_chunk_packet; /* Index of first packet in the chunk */
  
  trak = file->atracks[track].track;
  stsc = &(trak->mdia.minf.stbl.stsc);

  if(chunk >= trak->mdia.minf.stbl.stco.total_entries)
    return 0;
    
  i = 0;
  stsc_index = 0;
  first_chunk_packet = 0;

  for(i = 0; i < chunk-1; i++)
    {
    if((stsc_index < stsc->total_entries-1) && (stsc->table[stsc_index+1].chunk-1 == i))
      stsc_index++;
    first_chunk_packet += stsc->table[stsc_index].samples;
    }

  /* Get offset */
  offset = trak->mdia.minf.stbl.stco.table[chunk-1].offset;
  for(i = 0; i < packet; i++)
    offset += trak->mdia.minf.stbl.stsz.table[first_chunk_packet+i].size;

  /* Get packet size */
  packet_size = trak->mdia.minf.stbl.stsz.table[first_chunk_packet+packet].size;
  
  /* Get number of audio samples */
  if(samples)
    *samples = get_uncompressed_samples(&trak->mdia.minf.stbl.stts,
                                        first_chunk_packet+packet, first_chunk_packet+packet+1);
  
  /* Read the data */
  if(*buffer_alloc < packet_size+16)
    {
    *buffer_alloc = packet_size + 128;
    *buffer = realloc(*buffer, *buffer_alloc);
    }
  //  fprintf(stderr, "Read VBR packet, offset: %llx, size: %x, samples: %d\n", offset, packet_size, *samples);
  quicktime_set_position(file, offset);
  quicktime_read_data(file, *buffer, packet_size);
  return packet_size;
  }

