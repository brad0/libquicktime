/*
 *   Codec file handling
 */

#include <stdio.h>
#include <string.h>


#include "lqt.h"

#include "lqt_codecinfo_private.h"

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

/* Module filename and module index */

static const char * module_filename_key  = "ModuleFilename: ";
static const char * module_index_key     = "ModuleIndex: ";
static const char * module_file_time_key = "FileTime: ";

/* Types for parameters */

static const char * type_int            = "Integer";
static const char * type_string         = "String";

static const char * num_encoding_parameters_key  = "NumEncodingParameters: ";
static const char * num_decoding_parameters_key  = "NumDecodingParameters: ";

static const char * value_key           = "Value: ";
static const char * min_value_key       = "ValueMin: ";
static const char * max_value_key       = "ValueMax: ";

static const char * real_name_key       = "RealName: ";


/* Returns a NULL terminated list of all codecs */

#define READ_BUFFER_SIZE 2048

#define CHECK_KEYWORD(key) (!strncmp(line, key, strlen(key)))

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
    case LQT_PARAMETER_STRING:
      ret->val_string = __lqt_strdup(pos);
      break;
    }
  }

static void read_parameter_info(FILE * input,
                                lqt_codec_parameter_info_t * info,
                                char * line)
  {
  char * pos;

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
      else if(!strcmp(pos, type_string))
        {
        info->type = LQT_PARAMETER_STRING;
        info->val_default.val_string = (char*)0;
        info->val_min.val_string = (char*)0;
        info->val_max.val_string = (char*)0;
        }
      }
    else if(CHECK_KEYWORD(real_name_key))
      {
      pos = line + strlen(type_key);
      info->real_name = __lqt_strdup(pos);
      }

    else if(CHECK_KEYWORD(value_key))
      {
      pos = line + strlen(value_key);
      read_parameter_value(pos, &(info->val_default), info->type);
      }

    else if(CHECK_KEYWORD(min_value_key))
      {
      pos = line + strlen(min_value_key);
      read_parameter_value(pos, &(info->val_min), info->type);
      }
    else if(CHECK_KEYWORD(max_value_key))
      {
      pos = line + strlen(max_value_key);
      read_parameter_value(pos, &(info->val_max), info->type);
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
    
    /* Module filename */
    
    else if(CHECK_KEYWORD(module_filename_key))
      {
      pos = line + strlen(module_filename_key);
      codec->module_filename = __lqt_strdup(pos);
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
      
      codec->fourccs = malloc(codec->num_fourccs * sizeof(char*));
      for(i = 0; i < codec->num_fourccs; i++)
        codec->fourccs[i] = malloc(5 * sizeof(char));
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
    
    /* Number of parameters */

    else if(CHECK_KEYWORD(num_encoding_parameters_key))
      {
      pos = line + strlen(num_encoding_parameters_key);
      codec->num_encoding_parameters = atoi(pos);
      if(codec->num_encoding_parameters)
        codec->encoding_parameters =
          malloc(codec->num_encoding_parameters *
                 sizeof(lqt_codec_parameter_info_t));
      else
        codec->encoding_parameters =
          (lqt_codec_parameter_info_t*)0;
      }
    else if(CHECK_KEYWORD(num_decoding_parameters_key))
      {
      pos = line + strlen(num_decoding_parameters_key);
      codec->num_decoding_parameters = atoi(pos);
      if(codec->num_decoding_parameters)
        codec->decoding_parameters =
          malloc(codec->num_decoding_parameters *
                 sizeof(lqt_codec_parameter_info_t));
      else
        codec->decoding_parameters =
          (lqt_codec_parameter_info_t*)0;
        
      }
    
    /* Read parameters */

    else if(CHECK_KEYWORD(begin_parameter_e_key))
      {
      read_parameter_info(input,
                          &(codec->encoding_parameters[encoding_parameters_read]),
                          line);
      encoding_parameters_read++;
      }

    else if(CHECK_KEYWORD(begin_parameter_d_key))
      {
      read_parameter_info(input,
                     &(codec->decoding_parameters[decoding_parameters_read]),
                     line);
      decoding_parameters_read++;
      }
    else if(CHECK_KEYWORD(end_codec_key))
      break;
    }
  }


lqt_codec_info_t * lqt_read_codec_file(const char * filename)
  {
  FILE * input;
  char * line;
  char * pos = (char*)0;
  lqt_codec_info_t * ret =     (lqt_codec_info_t *)0;
  lqt_codec_info_t * ret_end = (lqt_codec_info_t *)0;

#ifndef NDEBUG
  int num_audio_codecs = 0;
  int num_video_codecs = 0;
#endif

  
#ifndef NDEBUG
  fprintf(stderr, "Reading codec file %s...", filename);
#endif
  
  input = fopen(filename, "r");
 
  if(!input)
    {
#ifndef NDEBUG
    fprintf(stderr, "failed\n");
#endif
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

#ifndef NDEBUG
      if(ret_end->type == LQT_CODEC_AUDIO)
        num_audio_codecs++;
      else if(ret_end->type == LQT_CODEC_VIDEO)
        num_video_codecs++;
#endif
      ret_end->next = (lqt_codec_info_t*)0;
      }
    }

#ifndef NDEBUG
  fprintf(stderr, "done, found %d audio codecs, %d video codecs\n",
          num_audio_codecs, num_video_codecs);

#endif

  fclose(input);
  
  free(line);
  return ret;
  }

static void write_parameter_info(FILE * output,
                                 const lqt_codec_parameter_info_t * info,
                                 int encode)
  {
  const char * tmp = (const char*)0;
  
  fprintf(output, "%s%s\n",
          (encode ? begin_parameter_e_key : begin_parameter_d_key),
          info->name);
  fprintf(output, "%s%s\n", real_name_key, info->real_name);
  switch(info->type)
    {
    case LQT_PARAMETER_INT:
      tmp = type_int;
      break;
    case LQT_PARAMETER_STRING:
      tmp = type_string;
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
    case LQT_PARAMETER_STRING:
      fprintf(output, "%s%s\n", value_key, info->val_default.val_string);
      break;
    }

  fprintf(output, "%s\n", end_parameter_key);
    
  }


static void write_codec_info(const lqt_codec_info_t * info, FILE * output)
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

  fprintf(output, "%s%d\n", num_fourccs_key, info->num_fourccs);

  fprintf(output, "%s", fourccs_key);
  
  for(i = 0; i < info->num_fourccs; i++)
    fprintf(output, "0x%08X ", LQT_STRING_2_FOURCC(info->fourccs[i]));
  fprintf(output, "\n");

  fprintf(output, "%s%d\n", num_encoding_parameters_key,
          info->num_encoding_parameters);

  for(i = 0; i < info->num_encoding_parameters; i++)
    {
    write_parameter_info(output, &(info->encoding_parameters[i]), 1);
    }

  fprintf(output, "%s%d\n", num_decoding_parameters_key,
          info->num_decoding_parameters);
    
  for(i = 0; i < info->num_decoding_parameters; i++)
    {
    write_parameter_info(output, &(info->decoding_parameters[i]), 0);
    }

  /* Module filename and index */
  fprintf(output, "%s%s\n", module_filename_key, info->module_filename);
  fprintf(output, "%s%d\n", module_index_key, info->module_index);
  fprintf(output, "%s%u\n", module_file_time_key, info->file_time);
  

  
  fprintf(output, "%s\n", end_codec_key);
  }

void lqt_write_codec_file(const char * filename)
  {
  int i;
  FILE * output;

  lqt_codec_info_t * codec_info;
  
  output = fopen(filename, "w");

  if(!output)
    return;

  /*
   *  Write initial comment
   */
  
  fprintf(output, "# This is the codec database file for libquicktime\n\
# It is automatically generated and should not be edited.\n\
# If you canged it, and your libquicktime program doesn't work\n\
# anymore, delete it, and you will get a new one\n");

  codec_info = lqt_audio_codecs;
  
  for(i = 0; i < lqt_num_audio_codecs; i++)
    {
    write_codec_info(codec_info, output);
    codec_info = codec_info->next;
    }

  codec_info = lqt_video_codecs;
  for(i = 0; i < lqt_num_video_codecs; i++)
    {
    write_codec_info(codec_info, output);
    codec_info = codec_info->next;
    }
  fclose(output);
  }
