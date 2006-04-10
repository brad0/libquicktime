/* 	Qtinfo by Elliot Lee <sopwith@redhat.com> */

#include <quicktime/quicktime.h>
#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>

static void file_info(char *filename);

int main(int argc, char *argv[])
{
	int i;

	if(argc < 2) {
		printf("Usage: %s filename...\n", argv[0]);
		return 1;
	}

	for(i = 1; i < argc; i++) {
		file_info(argv[i]);
	}

	return 0;
}

static void
file_info(char *filename)
{
        const lqt_channel_t * channel_setup;
        int cmodel, channels;
        quicktime_t* qtfile;
	int i, j, n;
        char * str;
        int frame_duration, framerate_constant;
        qtfile = quicktime_open(filename, 1, 0);

	if(!qtfile) {
		printf("Couldn't open %s as a QuickTime file.\n", filename);
		return;
	}

	printf("\nFile %s:\n", filename);

        str = quicktime_get_copyright(qtfile);

        if (str)
          printf("    copyright: %s\n",str);
        str = quicktime_get_name(qtfile);
        if (str)
          printf("    name:      %s\n",str);
        str = quicktime_get_info(qtfile);
        if (str)
          printf("    info:      %s\n",str);

        str = lqt_get_author(qtfile);
        if (str)
          printf("    author:    %s\n",str);

        str = lqt_get_artist(qtfile);
        if (str)
          printf("    artist:    %s\n",str);

        str = lqt_get_album(qtfile);
        if (str)
          printf("    album:     %s\n",str);

        str = lqt_get_genre(qtfile);
        if (str)
          printf("    genre:     %s\n",str);

        str = lqt_get_track(qtfile);
        if (str)
          printf("    track:     %s\n",str);

        str = lqt_get_comment(qtfile);
        if (str)
          printf("    comment:   %s\n",str);

        
	n = quicktime_audio_tracks(qtfile);
	printf("  %d audio tracks.\n", n);
	for(i = 0; i < n; i++) {
        channels = quicktime_track_channels(qtfile, i);
        channel_setup = lqt_get_channel_setup(qtfile, i);
        printf("    %d channels, %d bits, sample rate %ld, length %ld samples, ",
		 channels,
		 quicktime_audio_bits(qtfile, i),
		 quicktime_sample_rate(qtfile, i),
		 quicktime_audio_length(qtfile, i));
        if(lqt_is_avi(qtfile))
          {
          printf("wav_id 0x%02x.\n", lqt_get_wav_id(qtfile, i));
          }
        else
          {
          printf("compressor %s.\n", quicktime_audio_compressor(qtfile, i));
          }
        printf("    Sample format: %s.\n",
               lqt_sample_format_to_string(lqt_get_sample_format(qtfile, i)));
        printf("    Channel setup: ");
        if(channel_setup)
          {
          for(j = 0; j < channels; j++)
            {
            printf(lqt_channel_to_string(channel_setup[j]));
            if(j < channels-1)
              printf(", ");
            }
          printf("\n");
          }
        else
          printf("Not available\n");
        printf("    %ssupported.\n",
               quicktime_supported_audio(qtfile, i)?"":"NOT ");
        }
        
	n = quicktime_video_tracks(qtfile);
	printf("  %d video tracks.\n", n);
	for(i = 0; i < n; i++)
          {
          frame_duration = lqt_frame_duration(qtfile, i, &framerate_constant);
          
          printf("    %dx%d, depth %d\n    rate %f [%d:%d] %sconstant\n    length %ld frames\n    compressor %s.\n",
                 quicktime_video_width(qtfile, i),
                 quicktime_video_height(qtfile, i),
                 quicktime_video_depth(qtfile, i),
                 quicktime_frame_rate(qtfile, i),
                 lqt_video_time_scale(qtfile, i),
                 frame_duration, (framerate_constant ? "" : "not "),
                 quicktime_video_length(qtfile, i),
                 quicktime_video_compressor(qtfile, i));
          cmodel = lqt_get_cmodel(qtfile, i);
          printf("    Native colormodel:  %s\n", lqt_colormodel_to_string(cmodel));
          printf("    Interlace mode:     %s\n", lqt_interlace_mode_to_string(lqt_get_interlace_mode(qtfile, i)));
          if(cmodel == BC_YUV420P)
            printf("    Chroma placement: %s\n", lqt_chroma_placement_to_string(lqt_get_chroma_placement(qtfile, i)));
          printf("    %ssupported.\n",
                 quicktime_supported_video(qtfile, i)?"":"NOT ");
          }
        quicktime_close(qtfile);
}

