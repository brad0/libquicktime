#include <quicktime/lqt.h>
#include <gtk/gtk.h>
#include <string.h>

#include "lqt_gtk.h"

static void parameter_menu_callback(GtkWidget * w, gpointer data)
  {
  int i;
  LqtGtkParameterWidget * p = (LqtGtkParameterWidget*)data;

  for(i = 0; i < p->parameter_info->num_stringlist_options; i++)
    {
    if(w == p->menuitems[i])
      {
      p->selected = i;
      break;
      }
    }
  }

/*
 *   Transfer parameters from the widgets to the default
 *   values in the parameter info.
 */

static void parameter_set_string(lqt_parameter_info_t * info,
                                 const char * str)
  {
  if(info->val_default.val_string)
    free(info->val_default.val_string);
  info->val_default.val_string = malloc(strlen(str)+1);
  strcpy(info->val_default.val_string, str);
  }

static void parameter_widget_apply(LqtGtkParameterWidget * w)
  {
  const char * ptr;
  switch(w->parameter_info->type)
    {
    case LQT_PARAMETER_STRINGLIST:
      ptr = w->parameter_info->stringlist_options[w->selected];
      parameter_set_string(w->parameter_info, ptr);
      break;
    case LQT_PARAMETER_STRING:
      ptr = gtk_entry_get_text(GTK_ENTRY(w->widget));
      parameter_set_string(w->parameter_info, ptr);
      break;
    case LQT_PARAMETER_INT:
      if((w->parameter_info->val_min.val_int == 0) &&
         (w->parameter_info->val_max.val_int == 1))
        {
        w->parameter_info->val_default.val_int =
          gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->widget));
        }
      else if((w->parameter_info->val_min.val_int <
               w->parameter_info->val_max.val_int))
        {
        w->parameter_info->val_default.val_int =
          (int)(GTK_ADJUSTMENT(w->adjustment)->value);
        }
      else
        {
        w->parameter_info->val_default.val_int =
          gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w->widget));
        }
    }
  
  }

LqtGtkParameterWidget *
lqtgtk_create_parameter_widget(lqt_parameter_info_t * info)
  {
  int i;
  int item_index;
  LqtGtkParameterWidget * ret = calloc(1, sizeof(LqtGtkParameterWidget));

  ret->parameter_info = info;
  
  switch(info->type)
    {
    case LQT_PARAMETER_INT:

      /* Boolean */
      if((info->val_min.val_int == 0) && (info->val_max.val_int == 1))
        {
        ret->widget = gtk_check_button_new_with_label(info->real_name);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->widget),
                                     info->val_default.val_int);
        }
      /* Integer with limits -> slider */
      else if(info->val_min.val_int < info->val_max.val_int)
        {
        ret->label = gtk_label_new(info->real_name);
        ret->adjustment = gtk_adjustment_new((gfloat) info->val_min.val_int,
                                             (gfloat) info->val_min.val_int,
                                             (gfloat) info->val_max.val_int,
                                             0.0,
                                             0.0,
                                             0.0);
        ret->widget = gtk_hscale_new(GTK_ADJUSTMENT(ret->adjustment));
        gtk_scale_set_value_pos(GTK_SCALE(ret->widget),
                                GTK_POS_LEFT);
        
        gtk_scale_set_digits(GTK_SCALE(ret->widget), 0);
        gtk_adjustment_set_value(GTK_ADJUSTMENT(ret->adjustment),
                                 info->val_default.val_int);
        }
      /* Spinbutton */
      else
        {
        ret->label = gtk_label_new(info->real_name);
        ret->adjustment = gtk_adjustment_new(0.0,
                                             -1.0e8,
                                             1.0e8,
                                             1.0,
                                             1.0,
                                             1.0);
        ret->widget = gtk_spin_button_new(GTK_ADJUSTMENT(ret->adjustment),
                                          0.0,
                                          0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ret->widget),
                                  info->val_default.val_int);
        }
      break;
    case LQT_PARAMETER_STRING:
      ret->label = gtk_label_new(info->real_name);
      ret->widget = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(ret->widget),
                         info->val_default.val_string);
      break;
    case LQT_PARAMETER_STRINGLIST:    /* String with options */
      ret->selected = 0;
      item_index = 0;
      ret->label = gtk_label_new(info->real_name);

      ret->menu = gtk_menu_new();
      ret->menuitems = calloc(info->num_stringlist_options,
                              sizeof(GtkWidget*));
      for(i = 0; i < info->num_stringlist_options; i++)
        {
        ret->menuitems[i] =
          gtk_menu_item_new_with_label(info->stringlist_options[i]);

        if(!strcmp(info->stringlist_options[i],
                   info->val_default.val_string))
          item_index = i;
        
        gtk_signal_connect(GTK_OBJECT(ret->menuitems[i]),
                           "activate",
                           GTK_SIGNAL_FUNC(parameter_menu_callback),
                           (gpointer)ret);
        gtk_widget_show(ret->menuitems[i]);
        gtk_menu_append(GTK_MENU(ret->menu), ret->menuitems[i]);
        }
      gtk_widget_show(ret->menu);

      ret->widget = gtk_option_menu_new();
      gtk_option_menu_set_menu(GTK_OPTION_MENU(ret->widget), ret->menu);
      gtk_option_menu_set_history(GTK_OPTION_MENU(ret->widget),
                                  item_index);
      break;
    }

  gtk_widget_set_usize(ret->widget, 100, ret->widget->requisition.height);
  gtk_widget_show(ret->widget);
  if(ret->label)
    {
    gtk_widget_show(ret->label);
    }
  return ret;
  }

/*
 *  Maybe empty function if we trust gtk's widget destroying mechanisms
 */ 

void
lqtgtk_destroy_parameter_widget(LqtGtkParameterWidget * w)
  {
  free(w);
  }

/*
 *  Create Codec config widget
 */

LqtGtkCodecConfigWidget *
lqtgtk_create_codec_config_widget(lqt_parameter_info_t * parameter_info,
                                  int num_parameters)
  {
  int i;
  LqtGtkCodecConfigWidget * ret = calloc(1, sizeof(LqtGtkCodecConfigWidget));

  ret->parameter_info = parameter_info;
  ret->num_parameters = num_parameters;

  /* Create the parameter widgets */

  ret->parameter_widgets = calloc(num_parameters,
                                  sizeof(LqtGtkParameterWidget *));

  ret->widget = gtk_table_new(num_parameters, 2, 0);
  gtk_container_set_border_width(GTK_CONTAINER(ret->widget),
                                 10);
  gtk_table_set_row_spacings(GTK_TABLE(ret->widget),
                             5);
  gtk_table_set_col_spacings(GTK_TABLE(ret->widget),
                             5);
    
  for(i = 0; i < num_parameters; i++)
    {
    ret->parameter_widgets[i] =
      lqtgtk_create_parameter_widget(&parameter_info[i]);

    /* Bool parameters have no labels */

    if(ret->parameter_widgets[i]->label)
      {    
      gtk_table_attach(GTK_TABLE(ret->widget),
                       ret->parameter_widgets[i]->label,
                       0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_attach_defaults(GTK_TABLE(ret->widget),
                                ret->parameter_widgets[i]->widget,
                                1, 2, i, i+1);
      }
    else
      {
      gtk_table_attach_defaults(GTK_TABLE(ret->widget),
                                ret->parameter_widgets[i]->widget,
                                0, 2, i, i+1);
      }
    }
  gtk_widget_show(ret->widget);
  return ret;
  }

/* 
 * 
 */

void lqtgtk_destroy_codec_config_widget(LqtGtkCodecConfigWidget * w)
  {
  int i;

  for(i = 0; i < w->num_parameters; i++)
    lqtgtk_destroy_parameter_widget(w->parameter_widgets[i]);
  
  free(w->parameter_widgets);
  free(w);
  }


/*
 *  Codec Browser
 */

static void browser_select_row_callback(GtkWidget * w,
                                        gint row,
                                        gint column,
                                        GdkEvent * event,
                                        gpointer data)
  {
  LqtGtkCodecBrowser * cb = (LqtGtkCodecBrowser *)data;

  if(cb->selected == -1)
    {
    gtk_widget_set_sensitive(cb->info_button, 1);
    gtk_widget_set_sensitive(cb->up_button, 1);
    gtk_widget_set_sensitive(cb->down_button, 1);
    }
  
  cb->selected = row;
  
  cb->codec_info = cb->codecs[row];

  if((cb->encode && cb->codec_info->num_encoding_parameters) ||
     (cb->decode && cb->codec_info->num_decoding_parameters))
    {
    gtk_widget_set_sensitive(cb->parameters_button, 1);
    }
  else
    {
    gtk_widget_set_sensitive(cb->parameters_button, 0);
    }
  }

static void browser_button_callback(GtkWidget * w, gpointer data)
  {
  LqtGtkCodecBrowser * cb = (LqtGtkCodecBrowser *)data;

  LqtGtkCodecConfigWindow * codec_config_window;
  LqtGtkCodecInfoWindow * codec_info_window;
  
  
  if(w == cb->up_button)
    {

    }
  else if(w == cb->down_button)
    {

    }
  else if(w == cb->parameters_button)
    {
    codec_config_window =
      lqtgtk_create_codec_config_window(cb->codec_info,
                                        cb->encode,
                                        cb->decode);
    lqtgtk_codec_config_window_run(codec_config_window);
    lqtgtk_destroy_codec_config_window(codec_config_window);
    }
  else if(w == cb->info_button)
    {
    codec_info_window =
      lqtgtk_create_codec_info_window(cb->codec_info);
    lqtgtk_codec_info_window_run(codec_info_window);
    }
  }

static char * list_title = "Installed codecs";

LqtGtkCodecBrowser * lqtgtk_create_codec_browser(lqt_codec_type type,
                                                 int encode, int decode)
  {
  LqtGtkCodecBrowser * ret = calloc(1, sizeof(LqtGtkCodecBrowser));

  ret->encode = encode;
  ret->decode = decode;
  ret->type   = type;
  
  ret->widget = gtk_table_new(4, 2, 0); 
  gtk_table_set_row_spacings(GTK_TABLE(ret->widget), 10);
  gtk_table_set_col_spacings(GTK_TABLE(ret->widget), 10);
  gtk_container_set_border_width(GTK_CONTAINER(ret->widget), 10);

  ret->list = gtk_clist_new_with_titles(1, &list_title);

  gtk_signal_connect(GTK_OBJECT(ret->list), "select_row",
                     browser_select_row_callback,
                     (gpointer)ret);
  
  gtk_widget_show(ret->list);
 
  ret->scrolledwindow =
    gtk_scrolled_window_new(gtk_clist_get_hadjustment(GTK_CLIST(ret->list)),
                            gtk_clist_get_vadjustment(GTK_CLIST(ret->list)));

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->scrolledwindow),
                                 GTK_POLICY_NEVER,
                                 GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(ret->scrolledwindow), ret->list);
  gtk_widget_show(ret->scrolledwindow);

  gtk_table_attach_defaults(GTK_TABLE(ret->widget), ret->scrolledwindow,
                            0, 1, 0, 4);
  
  ret->up_button =         gtk_button_new_with_label("Up");
  ret->down_button =       gtk_button_new_with_label("Down");

  ret->parameters_button = gtk_button_new_with_label("Parameters...");
  ret->info_button =       gtk_button_new_with_label("Info...");

  gtk_signal_connect(GTK_OBJECT(ret->up_button), "clicked",
                     GTK_SIGNAL_FUNC(browser_button_callback),
                     (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->down_button), "clicked",
                     GTK_SIGNAL_FUNC(browser_button_callback),
                     (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->parameters_button), "clicked",
                     GTK_SIGNAL_FUNC(browser_button_callback),
                     (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->info_button), "clicked",
                     GTK_SIGNAL_FUNC(browser_button_callback),
                     (gpointer)ret);

  gtk_widget_show(ret->up_button);
  gtk_widget_show(ret->down_button);
  gtk_widget_show(ret->parameters_button);
  gtk_widget_show(ret->info_button);
  
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->up_button, 1, 2, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->down_button, 1, 2, 1, 2,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->parameters_button, 1, 2, 2, 3,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->info_button, 1, 2, 3, 4,
                   GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(ret->widget);
  return ret;
  }

void lqtgtk_codec_browser_update(LqtGtkCodecBrowser * b)
  {
  int i;
  int num_codecs = 0;
  
  if(b->codecs)
    lqt_destroy_codec_info(b->codecs);
    
  if(b->type == LQT_CODEC_AUDIO)
    b->codecs = lqt_query_registry(1, 0, b->encode, b->decode);
  else
    b->codecs = lqt_query_registry(0, 1, b->encode, b->decode);
    
  gtk_clist_clear(GTK_CLIST(b->list));

  while(1)
    {
    if(b->codecs[num_codecs])
      num_codecs++;
    else
      break;
    }

  for(i = 0; i < num_codecs; i++)
    {
    gtk_clist_append(GTK_CLIST(b->list),
                     &(b->codecs[i]->long_name));
    }
  b->selected = -1;
  b->codec_info = b->codecs[0];

  gtk_widget_set_sensitive(b->info_button, 0);
  gtk_widget_set_sensitive(b->up_button, 0);
  gtk_widget_set_sensitive(b->down_button, 0);
  gtk_widget_set_sensitive(b->parameters_button, 0);
  }

void lqtgtk_destroy_codec_browser(LqtGtkCodecBrowser * b)
  {
  if(b->codecs)
    lqt_destroy_codec_info(b->codecs);
    
  free(b);
  }

static void codec_config_window_button_callback(GtkWidget * w, gpointer data)
  {
  LqtGtkCodecConfigWindow * ccw = (LqtGtkCodecConfigWindow *)data;
  if(w == ccw->apply_button)
    {
    lqtgtk_codec_config_window_apply(ccw);
    }
  if(w == ccw->close_button)
    {
    gtk_widget_hide(ccw->window);
    gtk_main_quit();
    }
  }

LqtGtkCodecConfigWindow *
lqtgtk_create_codec_config_window(lqt_codec_info_t * codec_info,
                                  int encode,
                                  int decode)
  {
  LqtGtkCodecConfigWindow * ret =
    calloc(1, sizeof(LqtGtkCodecConfigWindow));

  ret->codec_info = codec_info;
  
  if(encode)
    ret->encode_widget =
      lqtgtk_create_codec_config_widget(codec_info->encoding_parameters,
                                        codec_info->num_encoding_parameters);

  if(decode)
    ret->decode_widget =
      lqtgtk_create_codec_config_widget(codec_info->decoding_parameters,
                                        codec_info->num_decoding_parameters);

  if(encode && decode)
    {
    ret->hbox = gtk_hbox_new(1, 10);
    if(codec_info->num_encoding_parameters)
      {
      ret->encoding_frame = gtk_frame_new("Encoding Options");
      
      gtk_container_add(GTK_CONTAINER(ret->encoding_frame),
                       ret->encode_widget->widget);
      gtk_widget_show(ret->encoding_frame);
      gtk_box_pack_start_defaults(GTK_BOX(ret->hbox), ret->encoding_frame);
      }
    if(codec_info->num_decoding_parameters)
      {
      ret->decoding_frame = gtk_frame_new("Decoding Options");
      gtk_container_add(GTK_CONTAINER(ret->decoding_frame),
                        ret->decode_widget->widget);
      gtk_widget_show(ret->decoding_frame);
      gtk_box_pack_start_defaults(GTK_BOX(ret->hbox), ret->decoding_frame);
      }
    gtk_widget_show(ret->hbox);
    }
  
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(ret->window), codec_info->long_name);
  gtk_window_set_modal(GTK_WINDOW(ret->window), TRUE);
  
  ret->apply_button = gtk_button_new_with_label("Apply");
  ret->close_button = gtk_button_new_with_label("Close");

  gtk_signal_connect(GTK_OBJECT(ret->apply_button),
                     "clicked",
                     GTK_SIGNAL_FUNC(codec_config_window_button_callback),
                     (gpointer)ret);
  
  gtk_signal_connect(GTK_OBJECT(ret->close_button),
                     "clicked",
                     GTK_SIGNAL_FUNC(codec_config_window_button_callback),
                     (gpointer)ret);

  GTK_WIDGET_SET_FLAGS (ret->apply_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (ret->close_button, GTK_CAN_DEFAULT);

  gtk_widget_show(ret->apply_button);
  gtk_widget_show(ret->close_button);

  ret->buttonbox = gtk_hbutton_box_new();
  ret->mainbox = gtk_vbox_new(0, 5);
  gtk_container_set_border_width(GTK_CONTAINER(ret->mainbox), 10);

  if(encode && decode)
    {
    gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox), ret->hbox);
    }
  else if(encode)
    gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox),
                                ret->encode_widget->widget);
  else if(decode)
    gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox),
                                ret->decode_widget->widget);
  

  
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->close_button);

  gtk_widget_show(ret->buttonbox);
  gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox), ret->buttonbox);
  gtk_widget_show(ret->mainbox);

  gtk_container_add(GTK_CONTAINER(ret->window), ret->mainbox);
  return ret;
  }

void lqtgtk_codec_config_window_run(LqtGtkCodecConfigWindow *w)
  {
  gtk_widget_show(w->window);
  gtk_main();
  }

/* Apply all values into the libquicktime codec registry */

void lqtgtk_codec_config_window_apply(LqtGtkCodecConfigWindow *w)
  {
  int i;

  /*
   *  First, all parameter widgets transfer their values to the
   *  lqt_parameter_t structures
   */

  if(w->encode_widget)
    {
    for(i = 0; i < w->codec_info->num_encoding_parameters; i++)
      {
      parameter_widget_apply(w->encode_widget->parameter_widgets[i]);
      lqt_set_default_parameter(w->codec_info->type,
                                1, w->codec_info->name,
                                w->codec_info->encoding_parameters[i].name,
                                &(w->encode_widget->parameter_info[i].val_default));

      }
    }
  if(w->decode_widget)
    {
    for(i = 0; i < w->codec_info->num_decoding_parameters; i++)
      {
      parameter_widget_apply(w->decode_widget->parameter_widgets[i]);
      lqt_set_default_parameter(w->codec_info->type,
                                0, w->codec_info->name,
                                w->codec_info->decoding_parameters[i].name,
                                &(w->decode_widget->parameter_info[i].val_default));
      }
    }
  }

void lqtgtk_destroy_codec_config_window(LqtGtkCodecConfigWindow *w)
  {
  gtk_widget_destroy(w->window);
  free(w);
  }

#define STRING_TO_FOURCC( str ) \
  ( ( (uint32_t)(unsigned char)(str[0]) << 24 ) | \
    ( (uint32_t)(unsigned char)(str[1]) << 16 ) | \
    ( (uint32_t)(unsigned char)(str[2]) << 8 ) | \
    ( (uint32_t)(unsigned char)(str[3]) ) )

LqtGtkCodecInfoWidget *
lqtgtk_create_codec_info_widget(const lqt_codec_info_t * info)
  {
  int i;
  char * tmp1;
  char * tmp2;
  
  LqtGtkCodecInfoWidget * ret = calloc(1, sizeof(LqtGtkCodecInfoWidget));

  ret->real_name = gtk_label_new(info->long_name);
  ret->short_name_label = gtk_label_new("Internal name: ");
  ret->short_name = gtk_label_new(info->name);
  ret->description = gtk_label_new(info->description);

  ret->module_filename = gtk_label_new(info->module_filename);
  ret->module_filename_label = gtk_label_new("Module: ");
  
  gtk_widget_show(ret->real_name);
  gtk_widget_show(ret->short_name);
  gtk_widget_show(ret->short_name_label);
  gtk_widget_show(ret->description);

  gtk_widget_show(ret->module_filename);
  gtk_widget_show(ret->module_filename_label);
    
  /* Create the fourccs label */

  tmp1 = malloc(info->num_fourccs * 30);
  tmp2 = malloc(30);
  
  *tmp1 = '\0';
  
  for(i = 0; i < info->num_fourccs - 1; i++)
    {
    sprintf(tmp2, "0x%08X (%s)\n", STRING_TO_FOURCC(info->fourccs[i]),
            info->fourccs[i]);
    strcat(tmp1, tmp2);
    }

  /* Last one without newline */
  
  sprintf(tmp2, "0x%08X (%s)", STRING_TO_FOURCC(info->fourccs[i]),
          info->fourccs[i]);
  strcat(tmp1, tmp2);

  ret->fourccs_label = gtk_label_new(tmp1);
  gtk_widget_show(ret->fourccs_label);
  
  free(tmp1);
  free(tmp2);

  /* Create encoding colormodels label */

  if(info->num_encoding_colormodels)
    {
    tmp1 = malloc(info->num_encoding_colormodels * 30);
    tmp2 = malloc(30);
    
    *tmp1 = '\0';
    
    for(i = 0; i < info->num_fourccs - 1; i++)
      {
      sprintf(tmp2, "%s\n",
              lqt_colormodel_to_string(info->encoding_colormodels[i]));
      strcat(tmp1, tmp2);
      }
    
    /* Last one without newline */
    
    strcat(tmp1,
           lqt_colormodel_to_string(info->encoding_colormodels[info->num_encoding_colormodels-1]));
    
    ret->encoding_colormodels_label = gtk_label_new(tmp1);
    gtk_widget_show(ret->encoding_colormodels_label);
    
    free(tmp1);
    free(tmp2);

    ret->encoding_colormodels_frame = gtk_frame_new("Encoding Colormodels");
    
    gtk_container_add(GTK_CONTAINER(ret->encoding_colormodels_frame),
                      ret->encoding_colormodels_label);
    }
  
  /* Pack all widgets onto their containers */
      
  ret->fourccs_frame = gtk_frame_new("Fourccs");
  
  gtk_container_add(GTK_CONTAINER(ret->fourccs_frame), ret->fourccs_label);

  ret->label_table = gtk_table_new(5, 2, 0);

  gtk_table_attach_defaults(GTK_TABLE(ret->label_table),
                            ret->real_name, 0, 2, 0, 1);

  gtk_table_attach_defaults(GTK_TABLE(ret->label_table),
                            ret->short_name_label, 0, 1, 1, 2);

  gtk_table_attach_defaults(GTK_TABLE(ret->label_table),
                            ret->short_name, 1, 2, 1, 2);

  gtk_table_attach_defaults(GTK_TABLE(ret->label_table),
                            ret->module_filename_label, 0, 1, 2, 3);

  gtk_table_attach_defaults(GTK_TABLE(ret->label_table),
                            ret->module_filename, 1, 2, 2, 3);
 

  gtk_table_attach_defaults(GTK_TABLE(ret->label_table),
                            ret->description, 0, 2, 3, 4);

  gtk_widget_show(ret->label_table);

  ret->widget = ret->label_table;
  return ret;
  }

void lqtgtk_destroy_codec_info_widget(LqtGtkCodecInfoWidget * w)
  {
  free(w);
  }

static void codec_info_window_button_callback(GtkWidget * w, gpointer data)
  {
  LqtGtkCodecInfoWindow * ciw = (LqtGtkCodecInfoWindow *)data;
  gtk_widget_hide(ciw->window);
  lqtgtk_destroy_codec_info_window(ciw);
  }

LqtGtkCodecInfoWindow *
lqtgtk_create_codec_info_window(const lqt_codec_info_t *info)
  {
  LqtGtkCodecInfoWindow * ret = calloc(1, sizeof(LqtGtkCodecInfoWindow));
  ret->info_widget = lqtgtk_create_codec_info_widget(info);
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), info->long_name);

  ret->mainbox = gtk_vbox_new(0, 10);

  ret->close_button = gtk_button_new_with_label("Close");
  GTK_WIDGET_SET_FLAGS (ret->close_button, GTK_CAN_DEFAULT);

  gtk_signal_connect(GTK_OBJECT(ret->close_button), "clicked",
                     GTK_SIGNAL_FUNC(codec_info_window_button_callback),
                     (gpointer)ret);
    
  gtk_widget_show(ret->close_button);
  
  
  gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox), ret->info_widget->widget);
  gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox), ret->close_button);
  
  gtk_widget_show(ret->mainbox);
  
  gtk_container_add(GTK_CONTAINER(ret->window), ret->mainbox);
  
  return ret;
  }

void
lqtgtk_destroy_codec_info_window(LqtGtkCodecInfoWindow * w)
  {
  lqtgtk_destroy_codec_info_widget(w->info_widget);
  gtk_widget_destroy(w->window);
  free(w);
  }

void 
lqtgtk_codec_info_window_run(LqtGtkCodecInfoWindow * w)
  {
  gtk_widget_show(w->window);
  }

