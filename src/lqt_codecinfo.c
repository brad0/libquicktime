#include <string.h>

#include <dlfcn.h>

#include <pthread.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <quicktime/lqt.h>

#include <lqt_codecinfo_private.h>

/*
 *  Define LQT_LIBQUICKTIME to prevent compiling the
 *  get_codec_api_version() function
 */

#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

/* Public function (lqt.h) */

int lqt_get_codec_api_version() { return LQT_CODEC_API_VERSION; }

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
 *  Codec Registry
 */

int lqt_num_audio_codecs = 0;
int lqt_num_video_codecs = 0;

lqt_codec_info_t * lqt_audio_codecs = (lqt_codec_info_t*)0;
lqt_codec_info_t * lqt_video_codecs = (lqt_codec_info_t*)0;

static int mutex_initialized = 0;
pthread_mutex_t codecs_mutex;

/*
 *  Lock and unlock the codec registry
 */

void lqt_registry_lock()
  {
  if(!mutex_initialized)
    {
    pthread_mutex_init(&codecs_mutex, (pthread_mutexattr_t *)0);
    mutex_initialized = 1;

    /* We initialize the registry also */
    
    lqt_registry_init();
    }
  pthread_mutex_lock(&codecs_mutex);
  }

void lqt_registry_unlock()
  {
  pthread_mutex_unlock(&codecs_mutex);
  }

/* Free memory of parameter info */

void destroy_parameter_info(lqt_parameter_info_t * p)
  {
  int i;
  if(p->name)
    free(p->name);
  if(p->real_name)
    free(p->real_name);

  switch(p->type)
    {
    case LQT_PARAMETER_STRING:
      if(p->val_default.val_string)
        free(p->val_default.val_string);
      break;
    case LQT_PARAMETER_STRINGLIST:
      if(p->val_default.val_string)
        free(p->val_default.val_string);
      if(p->stringlist_options)
        {
        for(i = 0; i < p->num_stringlist_options; i++)
          free(p->stringlist_options[i]);
        free(p->stringlist_options);
        }
      break;
    }
  }

/* Free memory of codec info (public) */

void destroy_codec_info(lqt_codec_info_t * ptr)
  {
  int i;
  
  if(ptr->name)
    free(ptr->name);          /* Name of the codec              */
  if(ptr->long_name)          /* Long name of the codec         */
    free(ptr->long_name);
  if(ptr->description)        /* Long name of the codec         */
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

static void
copy_parameter_info(lqt_parameter_info_t * ret, const lqt_parameter_info_t * info)
  {
  int i;
  
  if(info->name)
    ret->name = __lqt_strdup(info->name);
  if(info->real_name)
    ret->real_name = __lqt_strdup(info->real_name);

  ret->type = info->type;

  switch(ret->type)
    {
    case LQT_PARAMETER_INT:
      ret->val_min.val_int = info->val_min.val_int;
      ret->val_max.val_int = info->val_max.val_int;
      ret->val_default.val_int = info->val_default.val_int;
      break;
    case LQT_PARAMETER_STRING:
      if(info->val_default.val_string)
        ret->val_default.val_string =
          __lqt_strdup(info->val_default.val_string);
      break;
    case LQT_PARAMETER_STRINGLIST: /* String with options */
      if(info->val_default.val_string)
        ret->val_default.val_string =
          __lqt_strdup(info->val_default.val_string);

      ret->num_stringlist_options = info->num_stringlist_options;
      ret->stringlist_options = calloc(ret->num_stringlist_options,
                                       sizeof(char*));

      for(i = 0; i < ret->num_stringlist_options; i++)
        ret->stringlist_options[i] =
          __lqt_strdup(info->stringlist_options[i]);

      break;
    }
  }

/*
 *  Copy codec Info
 */

static lqt_codec_info_t *
copy_codec_info(const lqt_codec_info_t * info)
  {
  int i;
  lqt_codec_info_t * ret = calloc(1, sizeof(lqt_codec_info_t));

  if(info->name)
    ret->name = __lqt_strdup(info->name);
  if(info->long_name)
    ret->long_name = __lqt_strdup(info->long_name);
  if(info->description)
    ret->description = __lqt_strdup(info->description);

  if(info->module_filename)
    ret->module_filename = __lqt_strdup(info->module_filename);

  
  ret->type = info->type;
  ret->direction = info->direction;
  
  
  ret->num_fourccs = info->num_fourccs;
  if(ret->num_fourccs)
    {
    ret->fourccs = malloc(ret->num_fourccs * sizeof(char*));
    for(i = 0; i < ret->num_fourccs; i++)
      ret->fourccs[i] = __lqt_strdup(info->fourccs[i]);
    }

  ret->num_encoding_colormodels = info->num_encoding_colormodels;
  if(ret->num_encoding_colormodels)
    {
    ret->encoding_colormodels =
      malloc(ret->num_encoding_colormodels * sizeof(int));
    for(i = 0; i < ret->num_encoding_colormodels; i++)
      ret->encoding_colormodels[i] = info->encoding_colormodels[i];
    }
  
  ret->num_encoding_parameters = info->num_encoding_parameters;
  
  if(ret->num_encoding_parameters)
    {
    ret->encoding_parameters =
      calloc(ret->num_encoding_parameters, sizeof(lqt_parameter_info_t));

    for(i = 0; i < ret->num_encoding_parameters; i++)
      copy_parameter_info(&(ret->encoding_parameters[i]),
                          &(info->encoding_parameters[i]));
    }

  ret->num_decoding_parameters = info->num_decoding_parameters;
  if(ret->num_decoding_parameters)
    {
    ret->decoding_parameters =
      calloc(ret->num_decoding_parameters, sizeof(lqt_parameter_info_t));

    for(i = 0; i < ret->num_decoding_parameters; i++)
      copy_parameter_info(&(ret->decoding_parameters[i]),
                          &(info->decoding_parameters[i]));
    }
  return ret;
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

  int codec_api_version_module;
  int codec_api_version_us = lqt_get_codec_api_version();
    
  int (*get_num_codecs)();
  int (*get_codec_api_version)();
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

  /* Check the api version */

  get_codec_api_version = (int (*)())(dlsym(module, "get_codec_api_version"));

  if(!get_codec_api_version)
    {
    fprintf(stderr, "module %s has to API version and is thus terribly old\n",
            plugin_filename);
    return ret;
    }

  codec_api_version_module = get_codec_api_version();

  if(codec_api_version_module != codec_api_version_us)
    {
    fprintf(stderr, "Codec interface version mismatch of module %s\n\
Module interface version       %d\n\
Libquicktime interface version %d\n", 
            plugin_filename,
            codec_api_version_module,
            codec_api_version_us);
    return ret;
    }
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

void lqt_registry_destroy()
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

void lqt_registry_init()
  {
  lqt_codec_info_t * file_codecs;
  lqt_codec_info_t * tmp_file_codecs;

  lqt_registry_lock();
  
  if(lqt_audio_codecs || lqt_video_codecs)
    {
    lqt_registry_unlock();
    return;
    }
  
  file_codecs = lqt_registry_read();

  /* Scan for the plugins, use cached values if possible */
  
  scan_for_plugins(PLUGIN_DIR, &file_codecs);

  /*
   *  If there were codecs in the database, which have
   *  disappeared, they must be deleted now
   */
  
  while(file_codecs)
    {
#ifndef NDEBUG
    fprintf(stderr, "Removing codec %s from registry\n", file_codecs->name);
#endif
    tmp_file_codecs = file_codecs;
    file_codecs = file_codecs->next;
    destroy_codec_info(tmp_file_codecs);
    }
  
  /*
   *  Write the file again, so we can use it the next time
   */
  lqt_registry_unlock();
  
  lqt_registry_write();

  
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

/* Thread save methods of getting codec infos */

lqt_codec_info_t * lqt_get_audio_codec_info_c(int index)
  {
  const lqt_codec_info_t * info;
  lqt_codec_info_t * ret;

  lqt_registry_lock();

  info = lqt_get_audio_codec_info(index);

  if(info)
    ret = copy_codec_info(info);
  else
    ret = (lqt_codec_info_t*)0;
  lqt_registry_unlock();
  return ret;
  }

lqt_codec_info_t * lqt_get_video_codec_info_c(int index)
  {
  const lqt_codec_info_t * info;
  lqt_codec_info_t * ret;

  lqt_registry_lock();

  info = lqt_get_video_codec_info(index);

  if(info)
    ret = copy_codec_info(info);
  else
    ret = (lqt_codec_info_t*)0;
  lqt_registry_unlock();
  return ret;
  }



static void 
create_parameter_info(lqt_parameter_info_t * ret,
                    const lqt_parameter_info_static_t * info)
  {
  int i;
  ret->name = __lqt_strdup(info->name);           /* Parameter name  */
  ret->real_name = __lqt_strdup(info->real_name); /* Parameter name  */

  ret->type = info->type;

  switch(ret->type)
    {
    case LQT_PARAMETER_INT:
      ret->val_default.val_int = info->val_default.val_int;
      ret->val_min.val_int = info->val_min.val_int;
      ret->val_max.val_int = info->val_max.val_int;
      break;
    case LQT_PARAMETER_STRING:
      ret->val_default.val_string = __lqt_strdup(info->val_default.val_string);
      break;
    case LQT_PARAMETER_STRINGLIST:
      ret->val_default.val_string = __lqt_strdup(info->val_default.val_string);
      if(!info->stringlist_options)
        {
        fprintf(stderr, "Stringlist parameter %s has NULL options\n",
                info->name);
        return;
        }

      /* Count the options */

      ret->num_stringlist_options = 0;
      
      while(1)
        {
        if(info->stringlist_options[ret->num_stringlist_options])
          ret->num_stringlist_options++;
        else
          break;
        }

      /* Now, copy them */

      ret->stringlist_options = malloc(ret->num_stringlist_options);
      for(i = 0; i < ret->num_stringlist_options; i++)
        {
          ret->stringlist_options[i] =
          __lqt_strdup(info->stringlist_options[i]);
        }
      break;
    default:
      break;
    }
  
  }

lqt_codec_info_t *
lqt_create_codec_info(const lqt_codec_info_static_t * template)
  {
  int i;
  lqt_codec_info_t * ret;

  if(!template->fourccs)
    {
    fprintf(stderr, "Codec %s has no fourccs defined\n", template->name);
    return (lqt_codec_info_t*)0;
    }
  
  ret = calloc(1, sizeof(lqt_codec_info_t));

  ret->name = __lqt_strdup(template->name);
  ret->long_name = __lqt_strdup(template->long_name);
  ret->description = __lqt_strdup(template->description);

  ret->type      = template->type;
  ret->direction = template->direction;
  
  ret->num_fourccs = 0;
  while(1)
    {
    if(template->fourccs[ret->num_fourccs])
      ret->num_fourccs++;
    else
      break;
    }

  ret->fourccs = malloc(ret->num_fourccs * sizeof(char*));
  for(i = 0; i < ret->num_fourccs; i++)
    ret->fourccs[i] = __lqt_strdup(template->fourccs[i]);

  ret->num_encoding_colormodels = 0;

  if(template->encoding_colormodels)
    {
    while(1)
      {
      if(template->encoding_colormodels[ret->num_encoding_colormodels] !=
         LQT_COLORMODEL_NONE)
        ret->num_encoding_colormodels++;
      else
        break;
      }
    ret->encoding_colormodels =
      malloc(ret->num_encoding_colormodels * sizeof(int));
    for(i = 0; i < ret->num_encoding_colormodels; i++)
      ret->encoding_colormodels[i] = template->encoding_colormodels[i];
    }
    
  if(template->encoding_parameters)
    {
    ret->num_encoding_parameters = 0;
    while(1)
      {
      if(template->encoding_parameters[ret->num_encoding_parameters].name)
        ret->num_encoding_parameters++;
      else
        break;
      }
    }

  if(ret->num_encoding_parameters)
    {
    ret->encoding_parameters =
      calloc(ret->num_encoding_parameters,
             sizeof(lqt_parameter_info_t));
    for(i = 0; i < ret->num_encoding_parameters; i++)
      {
      /* Copy parameter info */
      create_parameter_info(&(ret->encoding_parameters[i]),
                            &(template->encoding_parameters[i]));
      }
    }
  else
    {
    ret->encoding_parameters = (lqt_parameter_info_t*)0;
    }

  if(template->decoding_parameters)
    {
    ret->num_decoding_parameters = 0;
    while(1)
      {
      if(template->decoding_parameters[ret->num_decoding_parameters].name)
        ret->num_decoding_parameters++;
      else
        break;
      }
    }

  if(ret->num_decoding_parameters)
    {
    ret->decoding_parameters =
      calloc(ret->num_decoding_parameters,
             sizeof(lqt_parameter_info_t));
    for(i = 0; i < ret->num_decoding_parameters; i++)
      {
      /* Copy parameter info */
      create_parameter_info(&(ret->decoding_parameters[i]),
                            &(template->decoding_parameters[i]));
      }
    }
  else
    {
    ret->decoding_parameters = (lqt_parameter_info_t*)0;
    }
  return ret;
  }

static void dump_codec_parameter(lqt_parameter_info_t * p)
  {
  int i;
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
    case LQT_PARAMETER_STRINGLIST:
      fprintf(stderr, "Stringlist, Default Value : %s\n",
              (p->val_default.val_string ? p->val_default.val_string :
               "NULL"));
      fprintf(stderr, "Options: ");
      for(i = 0; i < p->num_stringlist_options; i++)
        fprintf(stderr, "%s ", p->stringlist_options[i]);
      fprintf(stderr, "\n");
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
    lqt_registry_init();

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
    lqt_registry_init();

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

/*
 *  Query codec registry
 */

lqt_codec_info_t ** lqt_query_registry(int audio, int video,
                                       int encode, int decode)
  {
  lqt_codec_info_t ** ret;
  const lqt_codec_info_t * info;
  int num_codecs = 0, num_added = 0, i;
  lqt_registry_lock();

  if(audio)
    {
    for(i = 0; i < lqt_num_audio_codecs; i++)
      {
      info = lqt_get_audio_codec_info(i);
      if((encode && (info->direction != LQT_DIRECTION_DECODE)) ||
         (decode && (info->direction != LQT_DIRECTION_ENCODE)))
        num_codecs++;
      }
    }
  if(video)
    {
    for(i = 0; i < lqt_num_video_codecs; i++)
      {
      info = lqt_get_video_codec_info(i);
      if((encode && (info->direction != LQT_DIRECTION_DECODE)) ||
         (decode && (info->direction != LQT_DIRECTION_ENCODE)))
        num_codecs++;
      }
    }

  ret = calloc(num_codecs+1, sizeof(lqt_codec_info_t*));

  if(audio)
    {
    for(i = 0; i < lqt_num_audio_codecs; i++)
      {
      info = lqt_get_audio_codec_info(i);
      if((encode && (info->direction != LQT_DIRECTION_DECODE)) ||
         (decode && (info->direction != LQT_DIRECTION_ENCODE)))
        {
        ret[num_added] = copy_codec_info(info);
        num_added++;
        }
      }
    }
  if(video)
    {
    for(i = 0; i < lqt_num_video_codecs; i++)
      {
      info = lqt_get_video_codec_info(i);
      if((encode && (info->direction != LQT_DIRECTION_DECODE)) ||
         (decode && (info->direction != LQT_DIRECTION_ENCODE)))
        {
        ret[num_added] = copy_codec_info(info);
        num_added++;
        }
      }
    }
  lqt_registry_unlock();
  return ret;
  }

/*
 *  Find a codec by it's unique (short) name
 */

lqt_codec_info_t ** lqt_find_audio_codec_by_name(const char * name)
  {
  const lqt_codec_info_t * info;
  int i;
  lqt_codec_info_t ** ret = (lqt_codec_info_t**)0;
    
  lqt_registry_lock();

  info = lqt_get_audio_codec_info(0);

  for(i = 0; i < lqt_num_audio_codecs; i++)
    {
    if(!strcmp(info->name, name))
      {
      ret = calloc(2, sizeof(lqt_codec_info_t*));
      *ret = copy_codec_info(info);
      break;
      }
    else
      info = info->next;
    }
  lqt_registry_unlock();
  return ret;
  }

lqt_codec_info_t ** lqt_find_video_codec_by_name(const char * name)
  {
  const lqt_codec_info_t * info;
  int i;
  lqt_codec_info_t ** ret = (lqt_codec_info_t**)0;
    
  lqt_registry_lock();

  info = lqt_get_video_codec_info(0);

  for(i = 0; i < lqt_num_video_codecs; i++)
    {
    if(!strcmp(info->name, name))
      {
      ret = calloc(2, sizeof(lqt_codec_info_t*));
      *ret = copy_codec_info(info);
      break;
      }
    else
      info = info->next;
    }
  lqt_registry_unlock();
  return ret;
  }

/*
 *  Get infos about the Codecs of a file
 *  To be called after quicktime_open() when reading
 *  or quicktime_set_audio()/quicktime_set_video() when writing
 */

lqt_codec_info_t ** lqt_audio_codec_from_file(quicktime_t * file, int track)
  {
  char * name = ((quicktime_codec_t*)(file->atracks[track].codec))->codec_name;
  return lqt_find_audio_codec_by_name(name);
  }

lqt_codec_info_t ** lqt_video_codec_from_file(quicktime_t * file, int track)
  {
  char * name = ((quicktime_codec_t*)(file->vtracks[track].codec))->codec_name;
  return lqt_find_video_codec_by_name(name);
  }

/*
 *  Destroys the codec info structure returned by the functions
 *  above
 */

void lqt_destroy_codec_info(lqt_codec_info_t ** info)
  {
  lqt_codec_info_t ** ptr = info;

  if(!ptr)
    return;
  
  while(*ptr)
    {
    destroy_codec_info(*ptr);
    ptr++;
    }
  free(info);
  }

void lqt_set_default_parameter(lqt_codec_type type, int encode,
                               const char * codec_name,
                               const char * parameter_name,
                               lqt_parameter_value_t * val)
  {
  int i, imax, parameter_found = 0;
  
  lqt_codec_info_t * codec_info;
  lqt_parameter_info_t * parameter_info;
  
  lqt_registry_lock();

  if(type == LQT_CODEC_AUDIO)
    codec_info = lqt_audio_codecs;
  else
    codec_info = lqt_video_codecs;

  /* Search codec */

  while(codec_info)
    {
    if(!strcmp(codec_name, codec_info->name))
      break;
    codec_info = codec_info->next;
    }

  if(!codec_info)
    {
    fprintf(stderr, "lqt_set_default_parameter: No %s codec %s found\n",
            ((type == LQT_CODEC_AUDIO) ? "audio" : "video"),  codec_name);
    lqt_registry_unlock();
    return;
    }

  /* Search parameter */

  if(encode)
    {
    imax = codec_info->num_encoding_parameters;
    parameter_info = codec_info->encoding_parameters;
    }
  else
    {
    imax = codec_info->num_decoding_parameters;
    parameter_info = codec_info->decoding_parameters;
    }

  for(i = 0; i < imax; i++)
    {
    if(!strcmp(parameter_info[i].name, parameter_name))
      {
      parameter_found = 1;
      break;
      }
    }

  if(!parameter_found)
    {
    fprintf(stderr, "lqt_set_default_parameter: No parameter %s for codec \
%s found\n",
            parameter_name,
            codec_name);
    lqt_registry_unlock();
    return;
    }

  /* Set the value */

  switch(parameter_info[i].type)
    {
    case LQT_PARAMETER_INT:
      parameter_info[i].val_default.val_int = val->val_int;
#ifndef NDEBUG
  fprintf(stderr,
          "%s parameter %s for codec %s (value: %d) stored in registry\n",
          (encode ? "Encoding" : "Decoding"),
          parameter_name,
          codec_name, parameter_info[i].val_default.val_int);
#endif
      break;
    case LQT_PARAMETER_STRING:
    case LQT_PARAMETER_STRINGLIST:
      if(parameter_info[i].val_default.val_string)
        free(parameter_info[i].val_default.val_string);
      parameter_info[i].val_default.val_string =
        __lqt_strdup(val->val_string);
#ifndef NDEBUG
  fprintf(stderr,
          "%s parameter %s for codec %s (value: \"%s\") stored in registry\n",
          (encode ? "Encoding" : "Decoding"),
          parameter_name,
          codec_name, parameter_info[i].val_default.val_string);
#endif
      break;
    }


  lqt_registry_unlock();
  return;
  }



/***************************************************************
 * This will hopefully make the destruction for dynamic loading
 * (Trick comes from a 1995 version of the ELF Howto, so it
 * should work everywhere now)
 ***************************************************************/

#if defined(__GNUC__) && defined(__ELF__)

static void __lqt_cleanup_codecinfo() __attribute__ ((destructor));

static void __lqt_cleanup_codecinfo()
  {
#ifndef NDEBUG
  fprintf(stderr, "Deleting quicktime codecs\n");
#endif
  lqt_registry_destroy();
  }

#endif
