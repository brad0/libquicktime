/***************************************************
 * This program prints informations about all
 * installed libquicktime codecs
 ***************************************************/

#include <quicktime/quicktime.h>
#include <quicktime/lqt.h>
#include <lqt_codecinfo_private.h>

int main()
  {
  int num;
  int i;

  /* Initialize codecs */

  fprintf(stderr, "Libquicktime codec test, Codec API Version %d\n",
          lqt_get_codec_api_version());
  
  lqt_init_codec_info();
  
#if 0
  
  num = lqt_get_num_audio_codecs();

  for(i = 0; i < num; i++)
    {
    lqt_dump_codec_info(lqt_get_audio_codec_info(i));
    }

  num = lqt_get_num_video_codecs();

  for(i = 0; i < num; i++)
    {
    lqt_dump_codec_info(lqt_get_video_codec_info(i));
    }

  /* Write a codec file into quicktime_codecs.txt */

  lqt_write_codec_file("quicktime_codecs.txt");

#endif
  return 0;
  }
