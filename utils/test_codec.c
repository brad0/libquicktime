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
  
  lqt_registry_init();
  
#if 1
  
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

#endif
  return 0;
  }
