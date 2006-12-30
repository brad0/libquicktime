#ifndef __LQT_CHARSET
#define __LQT_CHARSET

#define LQT_UTF_8_16 "lqt_utf_8_16"

typedef struct lqt_charset_converter_s lqt_charset_converter_t;

lqt_charset_converter_t *
lqt_charset_converter_create(quicktime_t * file, const char * src_charset,
                             const char * dst_charset);

/* We convert all strings "in place" */

void lqt_charset_convert(lqt_charset_converter_t * cnv,
                         char ** string, int in_len, int * out_len);

/* Convert string and realloc result */

void lqt_charset_convert_realloc(lqt_charset_converter_t * cnv,
                                 const char * in_str, int in_len,
                                 char ** out_str, int * out_alloc, int * out_len);


void lqt_charset_converter_destroy(lqt_charset_converter_t *);

#endif
