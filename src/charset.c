#include <iconv.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


#include <quicktime.h>
#include <funcprotos.h>
#include <charset.h>

#define LOG_DOMAIN "charset"

struct lqt_charset_converter_s
  {
  iconv_t cd;
  quicktime_t * file; /* For logging */
  int utf_8_16;
  char * out_charset;

  char * in_buffer;
  int in_buffer_alloc;
  };

lqt_charset_converter_t *
lqt_charset_converter_create(quicktime_t * file, const char * src_charset, const char * dst_charset)
  {
  lqt_charset_converter_t * ret = calloc(1, sizeof(*ret));

  if(!strcmp(src_charset, LQT_UTF_8_16))
    {
    ret->out_charset = malloc(strlen(dst_charset)+1);
    strcpy(ret->out_charset, dst_charset);
    ret->utf_8_16 = 1;
    }
  else
    {
    ret->cd = iconv_open(dst_charset, src_charset);
    }
  ret->file = file;
  return ret;
  
  }

#define BYTES_INCREMENT 10

int do_convert(lqt_charset_converter_t * cnv, char * in_string, int len, int * out_len,
               char ** ret, int * ret_alloc)
  {

  char *inbuf;
  char *outbuf;
  int output_pos;

  size_t inbytesleft;
  size_t outbytesleft;

  /* Check for MP4 Unicode */
  if(cnv->utf_8_16 && !cnv->cd)
    {
    /* Byte order Little Endian */
    if((len > 1) &&
       ((uint8_t)in_string[0] == 0xff) &&
       ((uint8_t)in_string[1] == 0xfe))
      cnv->cd = iconv_open(cnv->out_charset, "UTF-16LE");
    /* Byte order Big Endian */
    else if((len > 1) &&
            ((uint8_t)in_string[0] == 0xfe) &&
            ((uint8_t)in_string[1] == 0xff))
      cnv->cd = iconv_open(cnv->out_charset, "UTF-16BE");
    /* UTF-8 */
    else if(!strcmp(cnv->out_charset, "UTF-8"))
      {
      if(*ret_alloc < len+1)
        {
        *ret_alloc = len + BYTES_INCREMENT;
        *ret       = realloc(*ret, *ret_alloc);
        }
      strncpy(*ret, in_string, len);
      (*ret)[len] = '\0';
      if(out_len)
        *out_len = len;
      return 1;
      }
    else
      {
      cnv->cd = iconv_open(cnv->out_charset, "UTF-8");
      }
    }
    
  
  if((*ret_alloc) < len + BYTES_INCREMENT)
    {
    *ret_alloc = len + BYTES_INCREMENT;
    *ret       = realloc(*ret, *ret_alloc);
    }
  inbytesleft  = len;
  outbytesleft = *ret_alloc;

  inbuf  = in_string;
  outbuf = *ret;
  while(1)
    {
    if(iconv(cnv->cd, &inbuf, &inbytesleft,
             &outbuf, &outbytesleft) == (size_t)-1)
      {
      switch(errno)
        {
        case E2BIG:
          output_pos = (int)(outbuf - *ret);

          *ret_alloc   += BYTES_INCREMENT;
          outbytesleft += BYTES_INCREMENT;

          *ret = realloc(*ret, *ret_alloc);
          outbuf = &((*ret)[output_pos]);
          break;
        case EILSEQ:
          lqt_log(cnv->file, LQT_LOG_ERROR, LOG_DOMAIN, "Invalid Multibyte sequence");
          return 0;
          break;
        case EINVAL:
          lqt_log(cnv->file, LQT_LOG_ERROR, LOG_DOMAIN, "Incomplete Multibyte sequence");
          return 0;
          break;
        }
      }
    if(!inbytesleft)
      break;
    }
  /* Zero terminate */

  output_pos = (int)(outbuf - *ret);

  if(outbytesleft < 2)
    {
    *ret_alloc+=2;
    *ret = realloc(*ret, *ret_alloc);
    outbuf = &((*ret)[output_pos]);
    }
  outbuf[0] = '\0';
  outbuf[1] = '\0';
  if(out_len)
    *out_len = outbuf - *ret;
  return 1;
  }

/* We convert all strings "in place" */

void lqt_charset_convert(lqt_charset_converter_t * cnv,
                         char ** str, int in_len, int * out_len)
  {
  char * new = (char*)0;
  int new_alloc = 0;
  if(!(*str))
    return;
  if(in_len < 0) in_len = strlen(*str);
  
  if(!do_convert(cnv, *str, in_len, out_len,
                 &new, &new_alloc))
    {
    if(new) free(new);
    return;
    }
  free(*str);
  *str = new;
  }

void lqt_charset_convert_realloc(lqt_charset_converter_t * cnv,
                                 const char * in_str, int in_len,
                                 char ** out_str, int * out_alloc, int * out_len)
  {
  if(in_len < 0) in_len = strlen(in_str);

  if(cnv->in_buffer_alloc < in_len + 2)
    {
    cnv->in_buffer_alloc = in_len + 128;
    cnv->in_buffer = realloc(cnv->in_buffer, cnv->in_buffer_alloc);
    }

  memcpy(cnv->in_buffer, in_str, in_len);
  cnv->in_buffer[in_len] = '\0';
  cnv->in_buffer[in_len+1] = '\0';
  
  do_convert(cnv, cnv->in_buffer, in_len, out_len,
             out_str, out_alloc);
  }

void lqt_charset_converter_destroy(lqt_charset_converter_t * cnv)
  {
  if(cnv->cd)
    iconv_close(cnv->cd);
  if(cnv->out_charset)
    free(cnv->out_charset);
  free(cnv);
  }
