/*
 * This one is used for auto generating
 * the codecs for the webpage
 */

#include "lqt_private.h"
#include <stdio.h>

static char * header = "<h1>Codecs</h1>\n\
Note that this list might not reflect the last stable release.\n\
Instead, we try to keep this in sync with the CVS version.\n\
<h2>Audio codecs</h2>\n\
<ul>\n";

static char * middle = "</ul>\n\
<h2>Video codecs</h2>\n\
<ul>\n";

static char * footer =
"</ul>\n";

int main(int argc, char ** argv)
  {
  int i;
  lqt_codec_info_t ** codecs;
  lqt_registry_init();

  /* Print header */

  printf(header);
  
  /* Print audio codecs */

  codecs = lqt_query_registry(1, 0, 1, 1);

  i = 0;
  while(codecs[i])
    {
    printf("<li><b>%s</b> ", codecs[i]->long_name);

    switch(codecs[i]->direction)
      {
      case LQT_DIRECTION_ENCODE:
        printf("Encode only\n");
        break;
      case LQT_DIRECTION_DECODE:
        printf("Decode only\n");
        break;
      case LQT_DIRECTION_BOTH:
        printf("Encode/Decode\n");
        break;
      }
    i++;
    }
  lqt_destroy_codec_info(codecs);
  printf(middle);

  codecs = lqt_query_registry(0, 1, 1, 1);

  i = 0;
  while(codecs[i])
    {
    printf("<li><b>%s</b> ", codecs[i]->long_name);

    switch(codecs[i]->direction)
      {
      case LQT_DIRECTION_ENCODE:
        printf("Encode only\n");
        break;
      case LQT_DIRECTION_DECODE:
        printf("Decode only\n");
        break;
      case LQT_DIRECTION_BOTH:
        printf("Encode/Decode\n");
        break;
      }
    i++;
    }
  lqt_destroy_codec_info(codecs);
  printf(footer);

  return 0;
  
  }
