#include <string.h>
#include <limits.h>

#include <dlfcn.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <quicktime/lqt.h>

#include <lqt_codecinfo_private.h>
#include <quicktime/lqt_codecapi.h>

/*
 *  Quick and dirty strdup function for the case it's not there
 */

static char * __lqt_strdup(const char * string)
  {
  char * ret = malloc(strlen(string)+1);
  strcpy(ret, string);
  return ret;
  }

/*
 *  Codec Database
 */

int lqt_num_audio_codecs = 0;
int lqt_num_video_codecs = 0;

lqt_codec_info_t * lqt_audio_codecs = (lqt_codec_info_t*)0;
lqt_codec_info_t * lqt_video_codecs = (lqt_codec_info_t*)0;

void destroy_parameter_info(lqt_codec_parameter_info_t * p)
  {
  if(p->name)
    free(p->name);
  if(p->real_name)
    free(p->real_name);
  if(p->type == LQT_PARAMETER_STRING)
    {
    if(p->val_default.val_string)
      free(p->val_default.val_string);
    }
  }

void destroy_codec_info(lqt_codec_info_t * ptr)
  {
  int i;
  
  if(ptr->name)
    free(ptr->name);               /* Name of the codec              */
  if(ptr->long_name)          /* Long name of the codec         */
    free(ptr->long_name);
  if(ptr->description)          /* Long name of the codec         */
    free(ptr->description);

  if(ptr->fourccs)
    {
    for(i = 0; i < ptr->num_fourccs; i++)
      free(ptr->fourccs[i]);
    free(ptr->fourccs);
    }

  if(ptr->encoding_parameters)
    {
    for(i = 0; i < ptr->num_encoding_parameters; i++)
      destroy_parameter_info(&(ptr->encoding_parameters[i]));
    free(ptr->encoding_parameters);
    }

  if(ptr->decoding_parameters)
    {
    for(i = 0; i < ptr->num_decoding_parameters; i++)
      destroy_parameter_info(&(ptr->decoding_parameters[i]));
    free(ptr->decoding_parameters);
    }
  free(ptr);
  }

/*
 *   Seek a codec in the database
 */

static lqt_codec_info_t * find_codec_by_filename(lqt_codec_info_t ** list,
                                                 const char * filename,
                                                 uint32_t time)
  {
  lqt_codec_info_t * new_list =     (lqt_codec_info_t*)0;
  lqt_codec_info_t * new_list_end = (lqt_codec_info_t*)0;
  lqt_codec_info_t * ret =          (lqt_codec_info_t*)0;
  lqt_codec_info_t * ret_end =      (lqt_codec_info_t*)0;

  lqt_codec_info_t * tmp_ptr;
  
  lqt_codec_info_t * ptr = *list;
 
  if(!ptr)
    return (lqt_codec_info_t*)0;

  while(ptr)
    {
    if(!strcmp(ptr->module_filename, filename))
      {
      /*
       *  File is there, but newer than our database entry
       *  -> Remove from the list
       */
      if(ptr->file_time < time)
        {
        tmp_ptr = ptr->next;

        destroy_codec_info(ptr);
        ptr = tmp_ptr;
        }
      else
        {
        if(ret)
          {
          ret_end->next = ptr;
          ret_end = ret_end->next;
          }
        else
          {
          ret = ptr;
          ret_end = ptr;
          }
        ptr = ptr->next;
        }
      }
    else /* Not our file, return to database */
      {
      if(new_list)
        {
        new_list_end->next = ptr;
        new_list_end = new_list_end->next;
        }
      else
        {
        new_list = ptr;
        new_list_end = ptr;
        }
      ptr = ptr->next;
      }
    }

  /* Prepare for returning */

  if(new_list)
    {
    new_list_end->next = (lqt_codec_info_t*)0;
    }
  *list = new_list;
  if(ret_end)
    ret_end->next = (lqt_codec_info_t*)0;
    
  return ret;
  }

static lqt_codec_info_t * load_codec_info_from_plugin(char * plugin_filename,
                                                      uint32_t time)
  {
  void * module;

  lqt_codec_info_t * ret_end;
  
  int i;

  int num_codecs;
  
  int (*get_num_codecs)();
  lqt_codec_info_t * (*get_codec_info)(int);
  
  lqt_codec_info_t * ret = (lqt_codec_info_t*)0;

#ifndef NDEBUG  
  fprintf(stderr, "Trying to load %s...", plugin_filename);
#endif
  
  module = dlopen(plugin_filename, RTLD_NOW);
  if(!module)
    {
#ifndef NDEBUG  
    fprintf(stderr, "\n");
#endif
    fprintf(stderr, "dlopen failed for %s: %s\n",
            plugin_filename, dlerror());
    return ret;
    }
#ifndef NDEBUG  
  else
    fprintf(stderr, "success\n");
#endif

  /* Now, get the codec parameters */

  get_num_codecs = (int (*)())(dlsym(module, "get_num_codecs"));

  if(!get_num_codecs)
    {
    fprintf(stderr, "Symbol %s not found in %s\n",
            "get_num_codecs", plugin_filename);
    return ret;
    }
  
  get_codec_info = (lqt_codec_info_t*(*)(int))(dlsym(module, "get_codec_info"));
  if(!get_codec_info)
    {
    fprintf(stderr, "Symbol %s not found in %s\n",
            "get_codec_info", plugin_filename);
    return ret;
    }

  /* Now, create the structure */

  num_codecs = get_num_codecs();
  if(!num_codecs)
    {
    fprintf(stderr, "No codecs found\n");
    return ret;
    }

  ret = get_codec_info(0);

  /* Complete the structure */

  ret_end = ret;
  
  ret_end->module_index = 0;
  /* Filename of the module  */
  ret_end->module_filename = __lqt_strdup(plugin_filename);
  ret_end->file_time = time; /* File modification time  */
  
  for(i = 1; i < num_codecs; i++)
    {
    ret_end->next = get_codec_info(i);
    ret_end = ret_end->next;
    
    ret_end->module_index = i;
    /* Filename of the module  */
    ret_end->module_filename = __lqt_strdup(plugin_filename);
    /* File modification time  */
    ret_end->file_time = time;
    }
  ret_end->next = (lqt_codec_info_t*)0;
  return ret;
  }

/*
 *   Register all codecs found in list
 */

static void register_codecs(lqt_codec_info_t * list,
                            lqt_codec_info_t ** audio_codecs_end,
                            lqt_codec_info_t ** video_codecs_end)
  {
  while(list)
    {
    if(list->type == LQT_CODEC_AUDIO)
      {
#ifndef NDEBUG
      fprintf(stderr, "Registering audio codec %s\n", list->name);
#endif
      if(*audio_codecs_end)
        {
        (*audio_codecs_end)->next = list;
        *audio_codecs_end = (*audio_codecs_end)->next;
        }
      else
        {
        lqt_audio_codecs = list;
            (*audio_codecs_end) = lqt_audio_codecs;
        }
      lqt_num_audio_codecs++;
      }
    if(list->type == LQT_CODEC_VIDEO)
      {
#ifndef NDEBUG
      fprintf(stderr, "Registering video codec %s\n", list->name);
#endif
      if((*video_codecs_end))
        {
        (*video_codecs_end)->next = list;
        (*video_codecs_end) = (*video_codecs_end)->next;
        }
      else
        {
        lqt_video_codecs = list;
        (*video_codecs_end) = lqt_video_codecs;
        }
      lqt_num_video_codecs++;
      }
    list = list->next;
    }
  }

static void scan_for_plugins(char * plugin_dir, lqt_codec_info_t ** database)
  {
  char * pos;
  
  char * filename;
  DIR * directory;
  struct dirent * directory_entry;
  struct stat status;

  lqt_codec_info_t * codecs;

  lqt_codec_info_t * video_codecs_end;
  lqt_codec_info_t * audio_codecs_end;

  filename = malloc(PATH_MAX * sizeof(char));
  
  /* Set the end pointers so we can quickly add codecs after */

#ifndef NDEBUG
  fprintf(stderr, "Scanning %s for plugins\n", plugin_dir);
#endif
  
  audio_codecs_end = lqt_audio_codecs;

  if(audio_codecs_end)
    while(audio_codecs_end->next)
      audio_codecs_end = audio_codecs_end->next;

  video_codecs_end = lqt_video_codecs;

  if(video_codecs_end)
    while(video_codecs_end->next)
      video_codecs_end = video_codecs_end->next;
    
  directory = opendir(PLUGIN_DIR);

  if(!directory)
    {
    fprintf(stderr, "Cannot open plugin directory %s\n\
Did you forget \"make install\"? You need it because\n\
libquicktime cannot load plugins out of the sourcetree\n", plugin_dir);
    return;
    }

  while(1)
    {
    directory_entry = readdir(directory);
    
    if(!directory_entry) /* We're done */
      break;

    /* Check the beginning of the filename */
    
    if(strncmp(directory_entry->d_name, "lqt_", 4))
      continue;

    /* Check the end of the filename -> filter out .la files */

    pos = strchr(directory_entry->d_name, '.');
    
    if(!pos)
      continue;
    
    if(strcmp(pos, ".so"))
      continue;
    
    /* Now, the file should be a valid plugin, construct the filename */
    
    strcpy(filename, plugin_dir);
    strcat(filename, "/");
    strcat(filename, directory_entry->d_name);
    
    stat(filename, &status);
    if(!S_ISREG(status.st_mode))
      continue;

#ifndef NDEBUG
    fprintf(stderr, "Found %s...", directory_entry->d_name);
#endif
    
    codecs = find_codec_by_filename(database, filename, status.st_ctime);
    
    if(codecs) /* Codec information found in database */
      {
#ifndef NDEBUG
      fprintf(stderr,
              "Already in database, registering all codecs\n");
#endif
      register_codecs(codecs,
                      &audio_codecs_end,
                      &video_codecs_end);
      }
    else /* Load the informations from the module */
      {
#ifndef NDEBUG
      fprintf(stderr,
              "Getting codec info from module\n");
#endif
      codecs = load_codec_info_from_plugin(filename, status.st_ctime);
      register_codecs(codecs,
                      &audio_codecs_end,
                      &video_codecs_end);
      
      }
    
    }
  free(filename);
  closedir(directory);
  }

void lqt_destroy_codec_info()
  {
  lqt_codec_info_t * tmp;

  while(lqt_audio_codecs)
    {
    tmp = lqt_audio_codecs->next;
    destroy_codec_info(lqt_audio_codecs);
    lqt_audio_codecs = tmp;
    }

  while(lqt_video_codecs)
    {
    tmp = lqt_video_codecs->next;
    destroy_codec_info(lqt_video_codecs);
    lqt_video_codecs = tmp;
    }

  lqt_num_video_codecs = 0;
  lqt_num_audio_codecs = 0;
  
  }

void lqt_init_codec_info()
  {
  lqt_codec_info_t * file_codecs;

  char * home_dir;
  char * filename_buffer = malloc(PATH_MAX + 1);

  if(lqt_audio_codecs || lqt_video_codecs)
    return;
  
  /* Obtain the home directory */

  home_dir = getenv("HOME");
    
  strcpy(filename_buffer, home_dir);

  strcat(filename_buffer, "/.libquicktime_codecs");

  file_codecs = lqt_read_codec_file(filename_buffer);

  /* Scan for the plugins, use cached values if possible */
  
  scan_for_plugins(PLUGIN_DIR, &file_codecs);

  /*
   *  Write the file again, so we can use it the next time
   */

  lqt_write_codec_file(filename_buffer);

  free(filename_buffer);
  }

/*
 *  Get the numbers of codecs
 */

int lqt_get_num_audio_codecs() { return lqt_num_audio_codecs; }

int lqt_get_num_video_codecs() { return lqt_num_video_codecs; }

/*
 *   Get corresponding info structures
 *   These point to the original database entries,
 *   so they are returned as const here
 */

const lqt_codec_info_t * lqt_get_audio_codec_info(int index)
  {
  const lqt_codec_info_t * ret;
  int i;

  if((index < 0) || (index >= lqt_num_audio_codecs))
    return (lqt_codec_info_t *)0;

  ret = lqt_audio_codecs;
  
  for(i = 0; i < index; i++)
    ret = ret->next;
  
  return ret;
  }


const lqt_codec_info_t * lqt_get_video_codec_info(int index)
  {
  const lqt_codec_info_t * ret;
  int i;

  if((index < 0) || (index >= lqt_num_video_codecs))
    return (lqt_codec_info_t *)0;

  ret = lqt_video_codecs;
  
  for(i = 0; i < index; i++)
    ret = ret->next;
  
  return ret;
  }

static void 
copy_parameter_info(lqt_codec_parameter_info_t * ret,
                    const lqt_codec_parameter_info_t * i)
  {
  ret->name = __lqt_strdup(i->name); /* Parameter name  */
  ret->real_name = __lqt_strdup(i->real_name); /* Parameter name  */

  ret->type = i->type;

  switch(ret->type)
    {
    case LQT_PARAMETER_INT:
      ret->val_default.val_int = i->val_default.val_int;
      ret->val_min.val_int = i->val_min.val_int;
      ret->val_max.val_int = i->val_max.val_int;
      break;
    case LQT_PARAMETER_STRING:
      ret->val_default.val_string = __lqt_strdup(i->val_default.val_string);
      break;
    default:
      break;
    }
  }


lqt_codec_info_t *
lqt_create_codec_info(const lqt_codec_info_static_t * template,
                      char * fourccs[],
                      int num_fourccs,
                      const lqt_codec_parameter_info_t * encoding_parameters,
                      int num_encoding_parameters,
                      const lqt_codec_parameter_info_t * decoding_parameters,
                      int num_decoding_parameters)
  {
  int i;
  lqt_codec_info_t * ret = calloc(1, sizeof(lqt_codec_info_t));

  ret->name = __lqt_strdup(template->name);
  ret->long_name = __lqt_strdup(template->long_name);
  ret->description = __lqt_strdup(template->description);

  ret->type      = template->type;
  ret->direction = template->direction;

  ret->num_fourccs = num_fourccs;
  if(num_fourccs)
    {
    ret->fourccs = malloc(num_fourccs * sizeof(char*));
    for(i = 0; i < num_fourccs; i++)
      ret->fourccs[i] = __lqt_strdup(fourccs[i]);
    }
  else /* This should never happen */
    ret->fourccs = (char**)0;
  
  ret->num_encoding_parameters = num_encoding_parameters;
  if(num_encoding_parameters)
    {
    ret->encoding_parameters =
      malloc(num_encoding_parameters * sizeof(lqt_codec_parameter_info_t));
    for(i = 0; i < num_encoding_parameters; i++)
      {
      /* Copy parameter info */
      copy_parameter_info(&(ret->encoding_parameters[i]),
                          &(encoding_parameters[i]));
      }
    }
  else
    {
    ret->encoding_parameters = (lqt_codec_parameter_info_t*)0;
    }

  ret->num_decoding_parameters = num_decoding_parameters;
  if(num_decoding_parameters)
    {
    ret->decoding_parameters = malloc(num_decoding_parameters *
                                      sizeof(lqt_codec_parameter_info_t));
    for(i = 0; i < num_decoding_parameters; i++)
      {
      /* Copy parameter info */
      copy_parameter_info(&(ret->decoding_parameters[i]),
                          &(decoding_parameters[i]));
      }
    }
  else
    {
    ret->decoding_parameters = (lqt_codec_parameter_info_t*)0;
    }

  
  return ret;
    
  }

static void dump_codec_parameter(lqt_codec_parameter_info_t * p)
  {
  fprintf(stderr, "Parameter: %s (%s) ", p->name,
          p->real_name);
  fprintf(stderr, "Type: ");
  switch(p->type)
    {
    case LQT_PARAMETER_INT:
      fprintf(stderr, "Integer, Default Value: %d ",
              p->val_default.val_int);

      if(p->val_min.val_int < p->val_max.val_int)
        fprintf(stderr, "(%d..%d)\n",
                p->val_min.val_int, p->val_max.val_int);
      else
        fprintf(stderr, "(unlimited)\n");
      break;
    case LQT_PARAMETER_STRING:
      fprintf(stderr, "String, Default Value : %s\n",
              (p->val_default.val_string ? p->val_default.val_string : "NULL"));
      break;
    }
  }


void lqt_dump_codec_info(const lqt_codec_info_t * info)
  {
  int i;
  fprintf(stderr, "Codec: %s (%s)\n", info->long_name, info->name);
  
  fprintf(stderr, "Type: %s Direction: ",
          (info->type == LQT_CODEC_AUDIO ? "Audio, " : "Video, ") );
  switch(info->direction)
    {
    case LQT_DIRECTION_ENCODE:
      fprintf(stderr, "Encode\n");
      break;
    case LQT_DIRECTION_DECODE:
      fprintf(stderr, "Decode\n");
      break;
    case LQT_DIRECTION_BOTH:
      fprintf(stderr, "Encode/Decode\n");
      break;
    }

  fprintf(stderr, "Description:\n%s\n", info->description);

  fprintf(stderr, "Four character codes: (fourccs)\n");
  for(i = 0; i < info->num_fourccs; i++)
    fprintf(stderr, "%s (0x%08x)\n", info->fourccs[i],
            LQT_STRING_2_FOURCC(info->fourccs[i]));

  if(!info->num_encoding_parameters)
    {
    fprintf(stderr, "No settable parameters for encoding\n");
    }
  else
    {
    for(i = 0; i < info->num_encoding_parameters; i++)
      dump_codec_parameter(&(info->encoding_parameters[i]));
    }

  if(!info->num_encoding_parameters)
    {
    fprintf(stderr, "No settable parameters for decoding\n");
    }
  else
    {
    for(i = 0; i < info->num_decoding_parameters; i++)
      dump_codec_parameter(&(info->decoding_parameters[i]));
    }
  fprintf(stderr, "Module filename: %s\nIndex inside module: %d\n",
          info->module_filename, info->module_index);
  }



#define MATCH_FOURCC(a, b) \
( ( a[0]==b[0] ) && \
  ( a[1]==b[1] ) &&\
  ( a[2]==b[2] ) &&\
  ( a[3]==b[3] ) )

/* 
 *   Find codecs: These replace get_acodec_index() and get_vcodec_index()
 *   This returns a pointer to the codec info or NULL if there is none.
 */

lqt_codec_info_t * lqt_find_audio_codec(char * fourcc, int encode)
  {
  int j;
  lqt_codec_info_t * ret;
  if(!lqt_audio_codecs)
    lqt_init_codec_info();

  ret = lqt_audio_codecs;
  
  while(ret)
    {
    for(j = 0; j < ret->num_fourccs; j++)
      {
      if(MATCH_FOURCC(ret->fourccs[j], fourcc))
        {
        if((encode) & (ret->direction != LQT_DIRECTION_DECODE))
          return ret;
        else if(ret->direction != LQT_DIRECTION_ENCODE)
          return ret;
        }
      }
    ret = ret->next;
    }
  return (lqt_codec_info_t*)0;
  }

lqt_codec_info_t * lqt_find_video_codec(char * fourcc, int encode)
  {
  int j;
  lqt_codec_info_t * ret;
  if(!lqt_video_codecs)
    lqt_init_codec_info();

  ret = lqt_video_codecs;
  
  while(ret)
    {
    for(j = 0; j < ret->num_fourccs; j++)
      {
      if(MATCH_FOURCC(ret->fourccs[j], fourcc))
        {
        if((encode) & (ret->direction != LQT_DIRECTION_DECODE))
          return ret;
        else if(ret->direction != LQT_DIRECTION_ENCODE)
          return ret;
        }
      }
    ret = ret->next;
    }
  return (lqt_codec_info_t*)0;
  }




/***************************************************************
 * This will hopefully make the destruction for dynamic loading
 * (Trick comes from a 1995 version of the ELF Howto, so it
 * should work everywhere now
 ***************************************************************/

#if defined(__GNUC__) && defined(__ELF__)

static void __lqt_cleanup_codecinfo() __attribute__ ((destructor));

static void __lqt_cleanup_codecinfo()
  {
#ifndef NDEBUG
  fprintf(stderr, "Deleting quicktime codecs\n");
#endif
  lqt_destroy_codec_info();
  }

#endif
