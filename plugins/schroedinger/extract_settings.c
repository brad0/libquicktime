/*
 *  Compile this file with
 *  gcc -g `pkg-config --cflags schroedinger-1.0` extract_settings.c `pkg-config --libs schroedinger-1.0`
 */

#include <schroedinger/schro.h>
#include <stdio.h>

int main(int argc, char ** argv)
  {
  int i, j, num;
  const SchroEncoderSetting * s;
  
  num = schro_encoder_get_n_settings();

  for(i = 0; i < num; i++)
    {
    s = schro_encoder_get_setting_info(i);

    fprintf(stderr, "Name: %s, type: ", s->name);
    
    switch(s->type)
      {
      case SCHRO_ENCODER_SETTING_TYPE_BOOLEAN:
        fprintf(stderr, "Boolean\n");
        fprintf(stderr, "  Default: %d\n", (int)(s->default_value));
        break;
      case SCHRO_ENCODER_SETTING_TYPE_INT:
        fprintf(stderr, "Integer\n");
        fprintf(stderr, "  Min: %d, max: %d\n", (int)s->min, (int)s->max);
        fprintf(stderr, "  Default: %d\n", (int)(s->default_value));
        break;
      case SCHRO_ENCODER_SETTING_TYPE_ENUM:
        fprintf(stderr, "Enum\n");
        j = 0;
        fprintf(stderr, "  Options:\n");
        while(j <= (int)s->max)
          {
          fprintf(stderr, "    %s\n", s->enum_list[j]);
          j++;
          }
        break;
      case SCHRO_ENCODER_SETTING_TYPE_DOUBLE:
        fprintf(stderr, "Double\n");
        fprintf(stderr, "  Min: %f, max: %f\n", s->min, s->max);
        fprintf(stderr, "  Default: %f\n", s->default_value);
        break;
      default:
        break;
      }
    }
  }
