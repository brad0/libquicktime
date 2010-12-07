#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
  

/*
 *  Widget for one parameter
 */

typedef struct
  {
  GtkWidget * label;
  GtkWidget * widget;
  GtkObject * adjustment;

  GtkWidget * menu;
  GtkWidget ** menuitems;
  
  lqt_parameter_info_t * parameter_info;

  int selected;
#if GTK_MINOR_VERSION < 12
  GtkTooltips * tooltips;
#endif
  } LqtGtkParameterWidget;

#if GTK_MINOR_VERSION < 12
LqtGtkParameterWidget *
lqtgtk_create_parameter_widget(lqt_parameter_info_t * info, GtkTooltips * tooltips, const char * gettext_domain);
#else
LqtGtkParameterWidget *
lqtgtk_create_parameter_widget(lqt_parameter_info_t * info, const char * gettext_domain);
#endif

void lqtgtk_destroy_parameter_widget(LqtGtkParameterWidget*);


void lqtgtk_set_parameter_value(LqtGtkParameterWidget * w,
                                const lqt_parameter_value_t * value);

void lqtgtk_get_parameter_value(LqtGtkParameterWidget * w,
                                lqt_parameter_value_t * value);

/*
 *  Widget for all parameters of one codec
 */

typedef struct
  {
  LqtGtkParameterWidget ** parameter_widgets;
  GtkWidget * widget;

  lqt_parameter_info_t * parameter_info;
  int num_parameters;
#if GTK_MINOR_VERSION < 12
  GtkTooltips * tooltips;
#endif
  } LqtGtkCodecConfigWidget;

LqtGtkCodecConfigWidget *
lqtgtk_create_codec_config_widget(lqt_parameter_info_t * parameter_info,
                                  int num_parameters,
                                  const char * gettext_domain);

void lqtgtk_destroy_codec_config_widget(LqtGtkCodecConfigWidget *);

/*
 *  Same as above but a complete window with buttons
 */

typedef struct
  {
  lqt_codec_info_t * codec_info;
  
  LqtGtkCodecConfigWidget * encode_widget;
  LqtGtkCodecConfigWidget * decode_widget;

  GtkWidget * encoding_frame;
  GtkWidget * decoding_frame;
  
  GtkWidget * window;
  GtkWidget * apply_button;
  GtkWidget * close_button;
  GtkWidget * restore_button;
  
  GtkWidget * buttonbox;
  GtkWidget * mainbox;
  GtkWidget * hbox;
  
  } LqtGtkCodecConfigWindow;

LqtGtkCodecConfigWindow *
lqtgtk_create_codec_config_window(lqt_codec_info_t * codec_info,
                                  int encode,
                                  int decode);

void lqtgtk_destroy_codec_config_window(LqtGtkCodecConfigWindow *);

void lqtgtk_codec_config_window_run(LqtGtkCodecConfigWindow *w);

void lqtgtk_codec_config_window_apply(LqtGtkCodecConfigWindow *w);

/*
 *  Widget, which displays all codec informations
 */
 
typedef struct
  {
  GtkWidget * label_table;
  GtkWidget * table;

  GtkWidget * widget;
 
  GtkWidget * real_name;

  GtkWidget * short_name;
  GtkWidget * short_name_label;

  GtkWidget * module_filename;
  GtkWidget * module_filename_label;
  
  
  GtkWidget * description;

  GtkWidget * fourccs_label;
  GtkWidget * fourccs_frame;

  GtkWidget * wav_ids_label;
  GtkWidget * wav_ids_frame;
  
  } LqtGtkCodecInfoWidget;

LqtGtkCodecInfoWidget *
lqtgtk_create_codec_info_widget(const lqt_codec_info_t *);

void lqtgtk_destroy_codec_info_widget(LqtGtkCodecInfoWidget *);

/*
 *  Same as above, but a complete window
 */

typedef struct
  {
  LqtGtkCodecInfoWidget * info_widget;
  GtkWidget * close_button;
  GtkWidget * window;
  GtkWidget * mainbox;
  } LqtGtkCodecInfoWindow;

LqtGtkCodecInfoWindow *
lqtgtk_create_codec_info_window(const lqt_codec_info_t *);

void
lqtgtk_destroy_codec_info_window(LqtGtkCodecInfoWindow *);

void
lqtgtk_codec_info_window_run(LqtGtkCodecInfoWindow *);

/*
 *  Codec browser
 */

typedef struct
  {
  GtkTreeModel * model;
  
  GtkWidget * list;        /* List of all codecs */
  GtkWidget * scrolledwindow;
  GtkWidget * up_button;
  GtkWidget * down_button;

  GtkWidget * parameters_button;
  GtkWidget * info_button;
  GtkWidget * widget;
  
  lqt_codec_info_t ** codecs;
  lqt_codec_info_t * codec_info;

  int encode;
  int decode;
  lqt_codec_type type;

  int selected;
  int num_codecs;
  } LqtGtkCodecBrowser;

LqtGtkCodecBrowser * lqtgtk_create_codec_browser(lqt_codec_type type,
                                                 int encode, int decode);

void lqtgtk_destroy_codec_browser(LqtGtkCodecBrowser * );

/* Update browser with current registry */

void lqtgtk_codec_browser_update(LqtGtkCodecBrowser * b);

/* Compatibility wrapper */
void lqtgtk_widget_set_can_default(GtkWidget * w, gboolean can_default);

#if 0 /* Not used */
  
/*
 *  Stuff for selecting encoders. This makes an option menu with the
 *  codecs as well as functional "info" and "options" buttons 
 */

typedef struct
  {
  GtkWidget * optionmenu;
  GtkWidget * info_button;
  GtkWidget * parameters_button;

  lqt_codec_info_t * current_encoder;
  
  /* Private members */

  lqt_codec_type type;
  
  lqt_codec_info_t ** encoders;

  int selected;

  int num_menu_items;
  int num_encoders;
  GtkWidget * menu;
  GtkWidget ** menu_items;
  
  } LqtGtkEncoderWidget;

LqtGtkEncoderWidget * lqtgtk_create_encoder_widget(lqt_codec_type);
void lqtgtk_destroy_encoder_widget(LqtGtkEncoderWidget *);

/* Sync the widget with the registry */

void lqtgtk_encoder_widget_update(LqtGtkEncoderWidget * ew);

/* Set and get the name of the current encoder */
  
const char * lqtgtk_encoder_widget_get_encoder(LqtGtkEncoderWidget * ew);

void lqtgtk_encoder_widget_set_encoder(LqtGtkEncoderWidget * ew,
                                       const char * name);

#endif
  
#ifdef __cplusplus
}
#endif /* __cplusplus */
