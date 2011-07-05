/*******************************************************************************
 lqt_codecfile.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2011 Members of the libquicktime project.

 Modified by Napoleon E. Cornejo
 May 4, 2007, San Salvador, El Salvador
 - Validated user home directory in create_filename() 

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this library; if not, write to the Free Software Foundation, Inc., 51
 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*******************************************************************************/ 

/*
 *   Codec file handling
 */

#include "lqt_private.h"
#include "lqt_codecinfo_private.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define LOG_DOMAIN "codecfile"

/*
 *  The keywords are defined globaly, so they are automatically
 *  the same in reading and writing functions
 */

static const char * begin_codec_key     = "BeginCodec: ";
static const char * end_codec_key       = "EndCodec";

/*
 *  Start string for parameter definition:
 *  These 2 strings (for en- and decoding MUST have the same length)
 */

static const char * begin_parameter_e_key = "BeginParameterE: ";
static const char * begin_parameter_d_key = "BeginParameterD: ";

static const char * end_parameter_key   = "EndParameter";

static const char * long_name_key       = "LongName: ";
static const char * description_key     = "Description: ";

static const char * type_key            = "Type: ";

/* Types for codecs */

static const char * type_audio          = "Audio";
static const char * type_video          = "Video";


/* Codec direction */

static const char * direction_key       = "Direction: ";
static const char * direction_encode    = "Encode";
static const char * direction_decode    = "Decode";
static const char * direction_both      = "Both";

static const char * num_fourccs_key     = "NumFourccs: ";
static const char * fourccs_key         = "Fourccs: ";

static const char * num_wav_ids_key     = "NumWavIds: ";
static const char * wav_ids_key         = "WavIds: ";

static const char * compatibility_key   = "Compatibility: ";
static const char * compression_id_key  = "CompressionID: ";

/* Module filename and module index */

static const char * module_filename_key  = "ModuleFilename: ";
static const char * module_index_key     = "ModuleIndex: ";
static const char * module_file_time_key = "FileTime: ";

static const char * gettext_domain_key  = "GettextDomain";
static const char * gettext_directory_key  = "GettextDirectory";


/* Types for parameters */

static const char * type_int            = "Integer";
static const char * type_float          = "Float";
static const char * type_string         = "String";
static const char * type_stringlist     = "Stringlist";
static const char * type_section        = "Section";

static const char * help_string_key     = "HelpString: ";
static const char * num_digits_key      = "NumDigits";


static const char * num_encoding_parameters_key  = "NumEncodingParameters: ";
static const char * num_decoding_parameters_key  = "NumDecodingParameters: ";

static const char * value_key           = "Value: ";
static const char * min_value_key       = "ValueMin: ";
static const char * max_value_key       = "ValueMax: ";

static const char * real_name_key       = "RealName: ";

static const char * num_options_key     = "NumOptions: ";
static const char * option_key          = "Options: ";
static const char * label_key           = "OptionLabels: ";

 /* Encoding colormodels */ 	 
	  	 
static const char *
num_encoding_colormodels_key = "NumEncodingColormodels: "; 	 

static const char *
encoding_colormodel_key =      "EncodingColormodel: "; 	 

static const char *
num_image_sizes_key = "NumImageSizes: "; 	 

static const char *
image_size_key =      "ImageSize: "; 	 

/* Codec order */

static const char * audio_order_key = "AudioOrder: ";
static const char * video_order_key = "VideoOrder: ";

#define READ_BUFFER_SIZE 2048

#define CHECK_KEYWORD(key) (!strncmp(line, key, strlen(key)))

static char * create_filename()
  {
  char * filename_buffer;
  /* Obtain the home directory */

  char * fdir;
  
  /* First look for a system-wide codec file if available.  If not
     look into users home */

  fdir = getenv("LQT_CODEC_FILE");
  if (fdir == NULL ) {
    
    lqt_log(NULL, LQT_LOG_DEBUG, LOG_DOMAIN, 
                    "no system-wide codec file. Looking in user's home.");
 
    fdir = getenv("HOME");
    if(!fdir)
      return NULL;
    filename_buffer = malloc(strlen(fdir) + 22);
    strcpy(filename_buffer, fdir);       
    strcat(filename_buffer, "/.libquicktime_codecs"); 
  
  } else 
     {
     filename_buffer = malloc(strlen(fdir) + 1);
     strcpy(filename_buffer, fdir);
     }
  return filename_buffer;
  
}

static char * __lqt_strdup(const char * string)
  {
  char * ret = malloc(strlen(string)+1);
  strcpy(ret, string);
  return ret;
  }

static void read_parameter_value(char * pos,
                                 lqt_parameter_value_t * ret,
                                 lqt_parameter_type_t type)
  {
  switch(type)
    {
    case LQT_PARAMETER_INT:
      ret->val_int = atoi(pos);
      break;
    case LQT_PARAMETER_FLOAT:
      ret->val_float = strtod(pos, (char**)0);
      break;
    case LQT_PARAMETER_STRING:
    case LQT_PARAMETER_STRINGLIST:
      ret->val_string = __lqt_strdup(pos);
      break;
    case LQT_PARAMETER_SECTION:
      break;
    }
  }

static char * convert_help_string(char * str)
  {
  char * ret;
  char * src_pos, *dst_pos;
  
  ret = malloc(strlen(str)+1);
  
  src_pos = str;
  dst_pos = ret;

  while(*src_pos != '\0')
    {
    if((src_pos[0] == '\\') && (src_pos[1] == 'n'))
      {
      *dst_pos = '\n';
      src_pos += 2;
      dst_pos++;
      }
    else
      {
      *dst_pos = *src_pos;
      src_pos++;
      dst_pos++;
      }
    }
  *dst_pos = '\0';
  free(str);
  return ret;
  }

static void read_parameter_info(FILE * input,
                                lqt_parameter_info_t * info,
                                char * line)
  {
  char * pos;
  int options_read = 0;
  int labels_read = 0;
  
  /* First, get the name */

  pos = line + strlen(begin_parameter_e_key);
  info->name = __lqt_strdup(pos);
    
  while(1)
    {
    fgets(line, READ_BUFFER_SIZE-1, input);
    if(feof(input))
      break;
    pos = strchr(line, '\n');
    if(pos)
      *pos = '\0';
    
    /* Now, go through the syntax */

    if(CHECK_KEYWORD(type_key))
      {
      pos = line + strlen(type_key);

      if(!strcmp(pos, type_int))
        {
        info->type = LQT_PARAMETER_INT;

        /*
         *  We set them here for the case, they are not set after
         *  (which can happen for min and max)
         */

        info->val_default.val_int = 0;
        info->val_min.val_int = 0;
        info->val_max.val_int = 0;
        }
      if(!strcmp(pos, type_float))
        {
        info->type = LQT_PARAMETER_FLOAT;

        /*
         *  We set them here for the case, they are not set after
         *  (which can happen for min and max)
         */

        info->val_default.val_float = 0;
        info->val_min.val_float = 0;
        info->val_max.val_float = 0;
        info->num_digits = 1;
        }

      /*
       *  Important: type_stringlist must be checked
       *  BEFORE type_string
       */

      else if(!strcmp(pos, type_stringlist))
        {
        info->type = LQT_PARAMETER_STRINGLIST;
        info->val_default.val_string = (char*)0;
        }
      else if(!strcmp(pos, type_string))
        {
        info->type = LQT_PARAMETER_STRING;
        info->val_default.val_string = (char*)0;
        }
      else if(!strcmp(pos, type_section))
        {
        info->type = LQT_PARAMETER_SECTION;
        info->val_default.val_string = (char*)0;
        }
      }
    else if(CHECK_KEYWORD(real_name_key))
      {
      pos = line + strlen(real_name_key);
      info->real_name = __lqt_strdup(pos);
      }

    else if(CHECK_KEYWORD(value_key))
      {
      pos = line + strlen(value_key);
      read_parameter_value(pos, &info->val_default, info->type);
      }

    else if(CHECK_KEYWORD(min_value_key))
      {
      pos = line + strlen(min_value_key);
      if(info->type == LQT_PARAMETER_INT)
        info->val_min.val_int = atoi(pos);
      else if(info->type == LQT_PARAMETER_FLOAT)
        info->val_min.val_float = strtod(pos, (char**)0);
      }

    else if(CHECK_KEYWORD(max_value_key))
      {
      pos = line + strlen(max_value_key);
      if(info->type == LQT_PARAMETER_INT)
        info->val_max.val_int = atoi(pos);
      else if(info->type == LQT_PARAMETER_FLOAT)
        info->val_max.val_float = strtod(pos, (char**)0);
      }
    else if(CHECK_KEYWORD(num_options_key))
      {
      pos = line + strlen(num_options_key);
      info->num_stringlist_options = atoi(pos);
      info->stringlist_options = calloc(info->num_stringlist_options,
                                        sizeof(char*));
      info->stringlist_labels = calloc(info->num_stringlist_options,
                                       sizeof(char*));
      }
    else if(CHECK_KEYWORD(option_key))
      {
      pos = line + strlen(option_key);
      info->stringlist_options[options_read] = __lqt_strdup(pos);
      options_read++;
      }
    else if(CHECK_KEYWORD(label_key))
      {
      pos = line + strlen(label_key);
      info->stringlist_labels[labels_read] = __lqt_strdup(pos);
      labels_read++;
      }
    else if(CHECK_KEYWORD(help_string_key))
      {
      pos = line + strlen(help_string_key);
      info->help_string = __lqt_strdup(pos);
      info->help_string = convert_help_string(info->help_string);
      }
    else if(CHECK_KEYWORD(num_digits_key))
      {
      pos = line + strlen(num_digits_key);
      info->num_digits = atoi(pos);
      }
    else if(CHECK_KEYWORD(end_parameter_key))
      break;
    }
  }

static void read_codec_info(FILE * input, lqt_codec_info_t * codec,
                            char * line)
  {
  char * pos, *rest;
  int i;
  
  int encoding_parameters_read = 0;
  int decoding_parameters_read = 0;
  int encoding_colormodels_read = 0;
  int image_sizes_read = 0;
  
  uint32_t tmp_fourcc;
  
  /* First, get the name */

  pos = line + strlen(begin_codec_key);
  codec->name = __lqt_strdup(pos);
  
  while(1)
    {
    fgets(line, READ_BUFFER_SIZE-1, input);
    if(feof(input))
      break;
    pos = strchr(line, '\n');
    if(pos)
      *pos = '\0';

    /* Long name */
    
    if(CHECK_KEYWORD(long_name_key))
      {
      pos = line + strlen(long_name_key);
      codec->long_name = __lqt_strdup(pos);
      }
    
    /* Description */
    
    else if(CHECK_KEYWORD(description_key))
      {
      pos = line + strlen(description_key);
      codec->description = __lqt_strdup(pos);
      }

    /* Type */
    
    else if(CHECK_KEYWORD(type_key))
      {
      pos = line + strlen(type_key);
      if(!strcmp(pos, type_audio))
        codec->type = LQT_CODEC_AUDIO;
      else if(!strcmp(pos, type_video))
        codec->type = LQT_CODEC_VIDEO;
      }

    /* Compression ID */

    else if(CHECK_KEYWORD(compression_id_key))
      {
      pos = line + strlen(compression_id_key);
      codec->compression_id =
        lqt_compression_id_from_string(pos);
      }
    
    /* Direction */
   
    else if(CHECK_KEYWORD(direction_key))
      {
      pos = line + strlen(direction_key);
      if(!strcmp(pos, direction_encode))
        codec->direction = LQT_DIRECTION_ENCODE;
      else if(!strcmp(pos, direction_decode))
        codec->direction = LQT_DIRECTION_DECODE;
      else if(!strcmp(pos, direction_both))
        codec->direction = LQT_DIRECTION_BOTH;
      }

    /* Compatibility flags */
    else if(CHECK_KEYWORD(compatibility_key))
      {
      pos = line + strlen(compatibility_key);
      codec->compatibility_flags = strtoul(pos, (char**)0, 16);
      }
    
    /* Module filename */
    
    else if(CHECK_KEYWORD(module_filename_key))
      {
      pos = line + strlen(module_filename_key);
      codec->module_filename = __lqt_strdup(pos);
      }

    /* Gettext domain */
    
    else if(CHECK_KEYWORD(gettext_domain_key))
      {
      pos = line + strlen(gettext_domain_key);
      codec->gettext_domain = __lqt_strdup(pos);
      }

    /* Gettext directory */
    
    else if(CHECK_KEYWORD(gettext_directory_key))
      {
      pos = line + strlen(gettext_directory_key);
      codec->gettext_directory = __lqt_strdup(pos);
      }
    
    /* Module Index */
    
    else if(CHECK_KEYWORD(module_index_key))
      {
      pos = line + strlen(module_index_key);
      codec->module_index = atoi(pos);
      }

    /* File modification time */
    
    else if(CHECK_KEYWORD(module_file_time_key))
      {
      pos = line + strlen(module_file_time_key);
      codec->file_time = strtoul(pos, (char**)0, 10);
      }
    
    /* Number of Fourccs */

    else if(CHECK_KEYWORD(num_fourccs_key))
      {
      pos = line + strlen(num_fourccs_key);
      codec->num_fourccs = atoi(pos);

      /* We allocate memory here */
      if(codec->num_fourccs)
        {
        codec->fourccs = malloc(codec->num_fourccs * sizeof(char*));
        for(i = 0; i < codec->num_fourccs; i++)
          codec->fourccs[i] = malloc(5 * sizeof(char));
        }
      }
    
    /* Fourccs */
    
    else if(CHECK_KEYWORD(fourccs_key))
      {
      pos = line + strlen(fourccs_key);
      for(i = 0; i < codec->num_fourccs; i++)
        {
        tmp_fourcc = strtoul(pos, &rest, 16);
        LQT_FOURCC_2_STRING(codec->fourccs[i], tmp_fourcc);
        if(rest == pos)
          break;
        pos = rest;
        }
      }

    /* Number of wav ids */

    else if(CHECK_KEYWORD(num_wav_ids_key))
      {
      pos = line + strlen(num_wav_ids_key);
      codec->num_wav_ids = atoi(pos);

      /* We allocate memory here */
      
      codec->wav_ids = malloc(codec->num_wav_ids * sizeof(int));
      }

    /* Wav ids */
    
    else if(CHECK_KEYWORD(wav_ids_key))
      {
      pos = line + strlen(wav_ids_key);
      for(i = 0; i < codec->num_wav_ids; i++)
        {
        codec->wav_ids[i] = strtoul(pos, &rest, 16);
        pos = rest;
        }
      }

    /* Number of encoding colormodels */ 	 
	  	 
    else if(CHECK_KEYWORD(num_encoding_colormodels_key))
      {
      pos = line + strlen(num_encoding_colormodels_key);
      codec->num_encoding_colormodels = atoi(pos);
      if(codec->num_encoding_colormodels)
        {
        codec->encoding_colormodels =
          malloc((codec->num_encoding_colormodels+1) *
                 sizeof(*codec->encoding_colormodels));
        /* terminate */
        codec->encoding_colormodels[codec->num_encoding_colormodels] = 
          LQT_COLORMODEL_NONE;
        }
      else 	 
        codec->encoding_colormodels = (int*)0; 	 
      } 	 

    /* Encoding colormodels */ 	 

    else if(CHECK_KEYWORD(encoding_colormodel_key)) 	 
      {
      pos = line + strlen(encoding_colormodel_key);
      codec->encoding_colormodels[encoding_colormodels_read] =
        lqt_string_to_colormodel(pos);
      encoding_colormodels_read++; 
      }

    /* Number of image sizes */ 	 
    
    else if(CHECK_KEYWORD(num_image_sizes_key))
      {
      pos = line + strlen(num_image_sizes_key);
      codec->num_image_sizes = atoi(pos);
      if(codec->num_image_sizes)
        codec->image_sizes =
          malloc(codec->num_image_sizes *
                 sizeof(*codec->image_sizes));
      else 	 
        codec->image_sizes = NULL; 	 
      } 	 

    /* Image size */ 	 

    else if(CHECK_KEYWORD(image_size_key)) 	 
      {
      pos = line + strlen(image_size_key);

      sscanf(pos, "%d %d", &codec->image_sizes[image_sizes_read].width,
             &codec->image_sizes[image_sizes_read].height);
      image_sizes_read++; 
      }
    
    /* Number of parameters */

    else if(CHECK_KEYWORD(num_encoding_parameters_key))
      {
      pos = line + strlen(num_encoding_parameters_key);
      codec->num_encoding_parameters = atoi(pos);
      if(codec->num_encoding_parameters)
        codec->encoding_parameters =
          calloc(codec->num_encoding_parameters,
                 sizeof(lqt_parameter_info_t));
      else
        codec->encoding_parameters =
          (lqt_parameter_info_t*)0;
      }
    else if(CHECK_KEYWORD(num_decoding_parameters_key))
      {
      pos = line + strlen(num_decoding_parameters_key);
      codec->num_decoding_parameters = atoi(pos);
      if(codec->num_decoding_parameters)
        codec->decoding_parameters =
          calloc(codec->num_decoding_parameters,
                 sizeof(lqt_parameter_info_t));
      else
        codec->decoding_parameters =
          (lqt_parameter_info_t*)0;
        
      }
    
    /* Read parameters */

    else if(CHECK_KEYWORD(begin_parameter_e_key))
      {
      read_parameter_info(input,
                          &codec->encoding_parameters[encoding_parameters_read],
                          line);
      encoding_parameters_read++;
      }

    else if(CHECK_KEYWORD(begin_parameter_d_key))
      {
      read_parameter_info(input,
                     &codec->decoding_parameters[decoding_parameters_read],
                     line);
      decoding_parameters_read++;
      }
    else if(CHECK_KEYWORD(end_codec_key))
      break;
    }
  
  }


lqt_codec_info_t * lqt_registry_read(char ** audio_order, char ** video_order)
  {
  FILE * input;
  char * line;
  char * pos = (char*)0;
  char * filename_buffer = create_filename();
  lqt_codec_info_t * ret =     (lqt_codec_info_t *)0;
  lqt_codec_info_t * ret_end = (lqt_codec_info_t *)0;

  if(!filename_buffer || (*filename_buffer == '\0'))
    return NULL;

  input = fopen(filename_buffer, "r");
 
  if(!input)
    {
    free(filename_buffer);
    return (lqt_codec_info_t*)0;
    }
  
  line = malloc(READ_BUFFER_SIZE);
  
  while(1)
    {
    fgets(line, READ_BUFFER_SIZE-1, input);
    if(feof(input))
      break;
    pos = strchr(line, '\n');
    if(pos)
      *pos = '\0';

    pos = line;
        
    /* Skip comment lines */

    if(*pos == '#')
      continue;

    else if(CHECK_KEYWORD(audio_order_key))
      {
      if(audio_order)
        {
        pos += strlen(audio_order_key);
        *audio_order = __lqt_strdup(pos);
        }
      continue;
      }
    else if(CHECK_KEYWORD(video_order_key))
      {
      if(video_order)
        {
        pos += strlen(video_order_key);
        *video_order = __lqt_strdup(pos);
        }
      continue;
      }
    
    else if(CHECK_KEYWORD(begin_codec_key))
      {
      if(!ret_end)
        {
        ret = calloc(1, sizeof(lqt_codec_info_t));
        ret_end = ret;
        }
      else
        {
        ret_end->next = calloc(1, sizeof(lqt_codec_info_t));
        ret_end = ret_end->next;
        }
      read_codec_info(input, ret_end, pos);

      ret_end->next = (lqt_codec_info_t*)0;
      }
    }


  fclose(input);
  free(filename_buffer);
  free(line);
  return ret;
  }

static void write_help_string(FILE * output, char * help_string)
  {
  int i, imax;
  fprintf(output, "%s", help_string_key);

  imax = strlen(help_string);
  for(i = 0; i < imax; i++)
    {
    if(help_string[i] == '\n')
      fprintf(output, "\\n");
    else
      fprintf(output, "%c", help_string[i]);
    }
  fprintf(output, "\n");
  }

static void write_parameter_info(FILE * output,
                                 const lqt_parameter_info_t * info,
                                 int encode)
  {
  const char * tmp = (const char*)0;
  int i;
  
  fprintf(output, "%s%s\n",
          (encode ? begin_parameter_e_key : begin_parameter_d_key),
          info->name);
  fprintf(output, "%s%s\n", real_name_key, info->real_name);
  switch(info->type)
    {
    case LQT_PARAMETER_INT:
      tmp = type_int;
      break;
    case LQT_PARAMETER_FLOAT:
      tmp = type_float;
      break;
    case LQT_PARAMETER_STRING:
      tmp = type_string;
      break;
    case LQT_PARAMETER_STRINGLIST:
      tmp = type_stringlist;
      break;
    case LQT_PARAMETER_SECTION:
      tmp = type_section;
      break;
    }

  fprintf(output, "%s%s\n", type_key, tmp);
  
  /*
   *   Print the value
   */
  
  switch(info->type)
    {
    case LQT_PARAMETER_INT:
      fprintf(output, "%s%d\n", value_key, info->val_default.val_int);

      if(info->val_min.val_int < info->val_max.val_int)
        {
        fprintf(output, "%s%d\n", min_value_key, info->val_min.val_int);
        fprintf(output, "%s%d\n", max_value_key, info->val_max.val_int);
        }

      break;
    case LQT_PARAMETER_FLOAT:
      fprintf(output, "%s%f\n", value_key, info->val_default.val_float);

      if(info->val_min.val_float < info->val_max.val_float)
        {
        fprintf(output, "%s%f\n", min_value_key, info->val_min.val_float);
        fprintf(output, "%s%f\n", max_value_key, info->val_max.val_float);
        }
      fprintf(output, "%s%d\n", num_digits_key, info->num_digits);
      break;
    case LQT_PARAMETER_STRING:
      fprintf(output, "%s%s\n", value_key, info->val_default.val_string);
      break;
    case LQT_PARAMETER_STRINGLIST:
      fprintf(output, "%s%s\n", value_key, info->val_default.val_string);

      /* 
       *  Print options
       */
      fprintf(output, "%s%d\n", num_options_key, info->num_stringlist_options);
      
      for(i = 0; i < info->num_stringlist_options; i++)
        fprintf(output, "%s%s\n", option_key, info->stringlist_options[i]);
      for(i = 0; i < info->num_stringlist_options; i++)
        fprintf(output, "%s%s\n", label_key, info->stringlist_labels[i]);
      break;
    case LQT_PARAMETER_SECTION:
      break;
    }

  if(info->help_string)
    {
    write_help_string(output, info->help_string);
    }
  
  fprintf(output, "%s\n", end_parameter_key);
    
  }

/* 
 *  Write codec info, return FALSE on error
 */ 

static int write_codec_info(const lqt_codec_info_t * info, FILE * output)
  {
  const char * tmp;

  int i;
  
  fprintf(output, "%s%s\n", begin_codec_key, info->name);
  fprintf(output, "%s%s\n", long_name_key, info->long_name);

  fprintf(output, "%s%s\n", description_key, info->description);

  tmp = NULL;
  
  switch(info->type)
    {
    case LQT_CODEC_AUDIO:
      tmp = type_audio;
      break;
    case LQT_CODEC_VIDEO:
      tmp = type_video;
      break;
    }

  if(tmp)
    fprintf(output, "%s%s\n", type_key, tmp);

  if(info->compression_id != LQT_COMPRESSION_NONE)
    {
    fprintf(output, "%s%s\n", compression_id_key,
            lqt_compression_id_to_string(info->compression_id));
    }
  
  switch(info->direction)
    {
    case LQT_DIRECTION_DECODE:
      tmp = direction_decode;
      break;
    case LQT_DIRECTION_ENCODE:
      tmp = direction_encode;
      break;
    case LQT_DIRECTION_BOTH:
      tmp = direction_both;
      break;
    }

  if(tmp)
    fprintf(output, "%s%s\n", direction_key, tmp);

  fprintf(output, "%s%08x\n", compatibility_key, info->compatibility_flags);
 
  
  if(info->num_fourccs)
    {
    fprintf(output, "%s%d\n", num_fourccs_key, info->num_fourccs);
    
    fprintf(output, "%s", fourccs_key);
    
    for(i = 0; i < info->num_fourccs; i++)
      fprintf(output, "0x%08X ", LQT_STRING_2_FOURCC(info->fourccs[i]));
    fprintf(output, "\n");
    }
  
  if(info->num_wav_ids)
    {
    fprintf(output, "%s%d\n", num_wav_ids_key, info->num_wav_ids);
    fprintf(output, "%s", wav_ids_key);
    for(i = 0; i < info->num_wav_ids; i++)
      fprintf(output, "0x%02X ", info->wav_ids[i]);
    fprintf(output, "\n");
    }
  
  fprintf(output, "%s%d\n", num_encoding_parameters_key,
          info->num_encoding_parameters);

  for(i = 0; i < info->num_encoding_parameters; i++)
    {
    write_parameter_info(output, &info->encoding_parameters[i], 1);
    }

  fprintf(output, "%s%d\n", num_decoding_parameters_key,
          info->num_decoding_parameters);
    
  for(i = 0; i < info->num_decoding_parameters; i++)
    {
    write_parameter_info(output, &info->decoding_parameters[i], 0);
    }
  
  if((info->type == LQT_CODEC_VIDEO) &&
     (info->direction != LQT_DIRECTION_DECODE))
    {
    fprintf(output, "%s%d\n", num_encoding_colormodels_key,
            info->num_encoding_colormodels);
    
    for(i = 0; i < info->num_encoding_colormodels; i++)
      {
      fprintf(output, "%s%s\n", encoding_colormodel_key,
              lqt_colormodel_to_string(info->encoding_colormodels[i]));
      }
    }

  if((info->type == LQT_CODEC_VIDEO) &&
     (info->direction != LQT_DIRECTION_DECODE))
    {
    fprintf(output, "%s%d\n", num_image_sizes_key,
            info->num_image_sizes);
    for(i = 0; i < info->num_image_sizes; i++)
      {
      fprintf(output, "%s%d %d\n", image_size_key,
              info->image_sizes[i].width, info->image_sizes[i].height);
      }
    }
  
  /* Module filename and index */
  fprintf(output, "%s%s\n", module_filename_key, info->module_filename);
  fprintf(output, "%s%d\n", module_index_key, info->module_index);
  fprintf(output, "%s%u\n", module_file_time_key, info->file_time);

  if(info->gettext_domain)
    fprintf(output, "%s%s\n", gettext_domain_key, info->gettext_domain);
  if(info->gettext_directory)
    fprintf(output, "%s%s\n", gettext_directory_key, info->gettext_directory);
  
  if(fprintf(output, "%s\n", end_codec_key) < 0)
    return 0;
  return 1;
  }

void lqt_registry_write()
  {
  int i;
  FILE * output;
  char * filename_buffer = create_filename(); 
  lqt_codec_info_t * codec_info;

  lqt_registry_lock();
  
  if(!filename_buffer || (*filename_buffer == '\0'))
    {
    lqt_log(NULL, LQT_LOG_ERROR, LOG_DOMAIN, "Codec registry filename could not be generated");
    return;
    }
  
  output = fopen(filename_buffer, "w");
  
  if(!output)
    {
    lqt_registry_unlock();
    free(filename_buffer);
    return;
    }

  /*
   *  Write initial comment
   */
  
  fprintf(output, "# This is the codec database file for libquicktime\n\
# It is automatically generated and should not be edited.\n\
# If you changed it and your libquicktime program doesn't work\n\
# anymore, delete it, and you will get a new one\n");

  /* Write the sort strings */

  if(lqt_num_audio_codecs)
    {
    codec_info = lqt_audio_codecs;
    
    fprintf(output, "%s", audio_order_key);
    for(i = 0; i < lqt_num_audio_codecs; i++)
      {
      fprintf(output, "%s", codec_info->name);
      if(i == lqt_num_audio_codecs - 1)
        fprintf(output, "\n");
      else
        fprintf(output, ",");
      codec_info = codec_info->next;
      }
    }

  if(lqt_num_video_codecs)
    {
    codec_info = lqt_video_codecs;
    fprintf(output, "%s", video_order_key);
    for(i = 0; i < lqt_num_video_codecs; i++)
      {
      fprintf(output, "%s", codec_info->name);
      if(i == lqt_num_video_codecs - 1)
        fprintf(output, "\n");
      else
        fprintf(output, ",");
      codec_info = codec_info->next;
      }
    }
  
  codec_info = lqt_audio_codecs;
  
  for(i = 0; i < lqt_num_audio_codecs; i++)
    {
    if(!write_codec_info(codec_info, output))
      goto fail;
    codec_info = codec_info->next;
    }

  codec_info = lqt_video_codecs;
  for(i = 0; i < lqt_num_video_codecs; i++)
    {
    if(!write_codec_info(codec_info, output))
      goto fail;
    codec_info = codec_info->next;
    }
  fclose(output);
  lqt_registry_unlock();
  free(filename_buffer);
  return;
fail:
  fclose(output);
  lqt_registry_unlock();
  free(filename_buffer);
  lqt_log(NULL, LQT_LOG_INFO, LOG_DOMAIN,
          "%s could not be written, deleting imcomplete file", filename_buffer);
  remove(filename_buffer);
  }
