
typedef struct
  {
  GtkWidget * label;
  
  GtkWidget * widget;
  
  GtkObject * adjustment;

  GtkWidget * menu;
  GtkWidget ** menuitems;
  
  lqt_parameter_info_t * parameter_info;

  int selected;
  
  } LqtGtkParameterWidget;

LqtGtkParameterWidget *
lqtgtk_create_parameter_widget(lqt_parameter_info_t * info);

void
lqtgtk_destroy_parameter_widget(LqtGtkParameterWidget*);


void lqtgtk_set_parameter_value(LqtGtkParameterWidget * w,
                                const lqt_parameter_value_t * value);

void lqtgtk_get_parameter_value(LqtGtkParameterWidget * w,
                                lqt_parameter_value_t * value);

typedef struct
  {
  LqtGtkParameterWidget ** parameter_widgets;
  GtkWidget * widget;

  lqt_parameter_info_t * parameter_info;
  int num_parameters;
  
  } LqtGtkCodecConfigWidget;

LqtGtkCodecConfigWidget *
lqtgtk_create_codec_config_widget(lqt_parameter_info_t * parameter_info,
                                  int num_parameters);

void lqtgtk_destroy_codec_config_widget(LqtGtkCodecConfigWidget *);

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

typedef struct
  {
  GtkWidget * label_table;

  GtkWidget * widget;
 
  GtkWidget * real_name;

  GtkWidget * short_name;
  GtkWidget * short_name_label;

  GtkWidget * module_filename;
  GtkWidget * module_filename_label;
  
  
  GtkWidget * description;

  GtkWidget * fourccs_label;
  GtkWidget * fourccs_frame;

  GtkWidget * encoding_colormodels_label;
  GtkWidget * encoding_colormodels_frame;

  
  } LqtGtkCodecInfoWidget;

LqtGtkCodecInfoWidget *
lqtgtk_create_codec_info_widget(const lqt_codec_info_t *);

void lqtgtk_destroy_codec_info_widget(LqtGtkCodecInfoWidget *);

typedef struct
  {
  LqtGtkCodecInfoWidget * info_widget;
  GtkWidget * close_button;
  GtkWidget * window;
  GtkWidget * mainbox;
  }LqtGtkCodecInfoWindow;

LqtGtkCodecInfoWindow *
lqtgtk_create_codec_info_window(const lqt_codec_info_t *);

void
lqtgtk_destroy_codec_info_window(LqtGtkCodecInfoWindow *);

void
lqtgtk_codec_info_window_run(LqtGtkCodecInfoWindow *);


typedef struct
  {
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
  
  } LqtGtkCodecBrowser;

LqtGtkCodecBrowser * lqtgtk_create_codec_browser(lqt_codec_type type,
                                                 int encode, int decode);

void lqtgtk_destroy_codec_browser(LqtGtkCodecBrowser * );

/* Update browser with current registry */

void lqtgtk_codec_browser_update(LqtGtkCodecBrowser * b);

