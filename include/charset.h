
typedef struct lqt_charset_converter_s lqt_charset_converter_t;

lqt_charset_converter_t *
lqt_charset_converter_create(quicktime_t * file, const char * src_charset,
                             const char * dst_charset);

/* We convert all strings "in place" */

void lqt_charset_convert(lqt_charset_converter_t * cnv,
                         char ** string, int in_len, int * out_len);

void lqt_charset_converter_destroy(lqt_charset_converter_t *);
