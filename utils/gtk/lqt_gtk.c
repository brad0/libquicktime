#include "lqt_private.h"
#include "lqt_gtk.h"
#include <stdlib.h>
#include <string.h>

#include <libintl.h>
#define _(str) dgettext(PACKAGE, str)
#define TR_DOM(str) dgettext(gettext_domain, str)

#define GTK_OPTION_MENU(x) x

enum 
{
LQGTK_LIST_CODEC_NAME_COLUMN_ID  = 0,
LQGTK_LIST_CODEC_INDEX_COLUMN_ID,
LQGTK_LIST_CODEC_NUM_COLS
};

void lqtgtk_widget_set_can_default(GtkWidget * w, gboolean can_default)
  {
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_default(w, can_default);
#else
  if(can_default)
    GTK_WIDGET_SET_FLAGS(w, GTK_CAN_DEFAULT);
  else
    GTK_WIDGET_UNSET_FLAGS(w, GTK_CAN_DEFAULT);
#endif
  }

static void parameter_combobox_callback(GtkWidget * w, gpointer data)
  {
  LqtGtkParameterWidget * p = (LqtGtkParameterWidget*)data;
  p->selected = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
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
      break;
    case LQT_PARAMETER_FLOAT:
      if((w->parameter_info->val_min.val_float <
          w->parameter_info->val_max.val_float))
        {
        w->parameter_info->val_default.val_float =
          GTK_ADJUSTMENT(w->adjustment)->value;
        }
      else
        {
        w->parameter_info->val_default.val_float =
          gtk_spin_button_get_value(GTK_SPIN_BUTTON(w->widget));
        }
    case LQT_PARAMETER_SECTION:
      break;
    }
  
  }


static void parameter_widget_update(LqtGtkParameterWidget * pw)
  {
  int i;
  switch(pw->parameter_info->type)
    {
    case LQT_PARAMETER_INT:
#ifndef NDEBUG
      fprintf(stderr, "Parameter: %s: %d\n", pw->parameter_info->name,
              pw->parameter_info->val_default.val_int);
#endif
      /* Boolean */
      if((pw->parameter_info->val_min.val_int == 0) && (pw->parameter_info->val_max.val_int == 1))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw->widget),
                                     pw->parameter_info->val_default.val_int);
      /* Integer with limits -> slider */
      else if(pw->parameter_info->val_min.val_int < pw->parameter_info->val_max.val_int)
        gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->adjustment),
                                 pw->parameter_info->val_default.val_int);
      /* Spinbutton */
      else
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(pw->widget),
                                  pw->parameter_info->val_default.val_int);
      break;
    case LQT_PARAMETER_FLOAT:
#ifndef NDEBUG
      fprintf(stderr, "Parameter: %s: %f\n", pw->parameter_info->name,
              pw->parameter_info->val_default.val_float);
#endif
      /* Float with limits -> slider */
      if(pw->parameter_info->val_min.val_float < pw->parameter_info->val_max.val_float)
        gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->adjustment),
                                 pw->parameter_info->val_default.val_float);
      /* Spinbutton */
      else
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(pw->widget),
                                  pw->parameter_info->val_default.val_float);
      break;
    case LQT_PARAMETER_STRING:
#ifndef NDEBUG
      fprintf(stderr, "Parameter: %s: %s\n", pw->parameter_info->name,
              pw->parameter_info->val_default.val_string);
#endif
      gtk_entry_set_text(GTK_ENTRY(pw->widget),
                         pw->parameter_info->val_default.val_string);
      break;
    case LQT_PARAMETER_STRINGLIST:    /* String with options */
#ifndef NDEBUG
      fprintf(stderr, "Parameter: %s: %s\n", pw->parameter_info->name,
              pw->parameter_info->val_default.val_string);
#endif
      for(i = 0; i < pw->parameter_info->num_stringlist_options; i++)
        {
        if(!strcmp(pw->parameter_info->stringlist_options[i],
                   pw->parameter_info->val_default.val_string))
          {
	  gtk_combo_box_set_active(GTK_COMBO_BOX(pw->widget), i);
          break;
          }
        }
    case LQT_PARAMETER_SECTION:
      break;
    }
  }

/*
 *  Really dirty trick to get tooltips for a GtkComboBox working:
 *  loop through all container children and set the tooltip for
 *  the child, which is a button
 */

static void
set_combo_tooltip(GtkWidget *widget, gpointer   data)
  {
  LqtGtkParameterWidget * w = (LqtGtkParameterWidget *)data;
  //  GtkTooltips *tooltips = (GtkTooltips *)data;

  if(GTK_IS_BUTTON (widget))
#if GTK_MINOR_VERSION < 12
    gtk_tooltips_set_tip(w->tooltips, widget,
                         w->parameter_info->help_string,
                         NULL);
#else
  gtk_widget_set_tooltip_text(widget, w->parameter_info->help_string);
#endif
  }

static void
realize_combo(GtkWidget *combo, gpointer   data)
  {
  LqtGtkParameterWidget * w = (LqtGtkParameterWidget *)data;

  gtk_container_forall (GTK_CONTAINER (combo),
                        set_combo_tooltip,
                        w);
  }

static void set_tooltip(LqtGtkParameterWidget * widget,
                        GtkWidget * w)
  {
#if GTK_MINOR_VERSION < 12
  gtk_tooltips_set_tip(widget->tooltips, w,
                       widget->parameter_info->help_string,
                       widget->parameter_info->help_string);
#else
  gtk_widget_set_tooltip_text(w, widget->parameter_info->help_string);
#endif
  }

#if GTK_MINOR_VERSION < 12
LqtGtkParameterWidget *
lqtgtk_create_parameter_widget(lqt_parameter_info_t * info, GtkTooltips * tooltips, const char * gettext_domain)
#else
LqtGtkParameterWidget *
lqtgtk_create_parameter_widget(lqt_parameter_info_t * info,
                               const char * gettext_domain)
#endif
  {
  int i;
  LqtGtkParameterWidget * ret = calloc(1, sizeof(LqtGtkParameterWidget));
#if GTK_MINOR_VERSION < 12
  ret->tooltips = tooltips;
#endif
  ret->parameter_info = info;
  
  switch(info->type)
    {
    case LQT_PARAMETER_INT:

      /* Boolean */
      if((info->val_min.val_int == 0) && (info->val_max.val_int == 1))
        {
        ret->widget =
          gtk_check_button_new_with_label(TR_DOM(info->real_name));
        if(info->help_string)
          set_tooltip(ret, ret->widget);
        }
      /* Integer with limits -> slider */
      else if(info->val_min.val_int < info->val_max.val_int)
        {
        ret->label = gtk_label_new(TR_DOM(info->real_name));
        gtk_misc_set_alignment(GTK_MISC(ret->label), 0.0, 0.5);
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
        if(info->help_string)
          set_tooltip(ret, ret->widget);
        }
      /* Spinbutton */
      else
        {
        ret->label = gtk_label_new(TR_DOM(info->real_name));
        gtk_misc_set_alignment(GTK_MISC(ret->label), 0.0, 0.5);
        ret->adjustment = gtk_adjustment_new(0.0,
                                             -1.0e8,
                                             1.0e8,
                                             1.0,
                                             1.0,
                                             0.0);
        ret->widget = gtk_spin_button_new(GTK_ADJUSTMENT(ret->adjustment),
                                          0.0,
                                          0);
        if(info->help_string)
          set_tooltip(ret, ret->widget);
        }
      break;
    case LQT_PARAMETER_FLOAT:

      /* Float with limits -> slider */
      if(info->val_min.val_float < info->val_max.val_float)
        {
        ret->label = gtk_label_new(TR_DOM(info->real_name));
        gtk_misc_set_alignment(GTK_MISC(ret->label), 0.0, 0.5);
        ret->adjustment = gtk_adjustment_new(info->val_min.val_float,
                                             info->val_min.val_float,
                                             info->val_max.val_float,
                                             0.0,
                                             0.0,
                                             0.0);
        
        ret->widget = gtk_hscale_new(GTK_ADJUSTMENT(ret->adjustment));

        gtk_scale_set_digits(GTK_SCALE(ret->widget),
                             info->num_digits);
        // fprintf(stderr, "** NUM DIGITS: %d\n", info->num_digits);
        
        gtk_scale_set_value_pos(GTK_SCALE(ret->widget),
                                GTK_POS_LEFT);
        
        if(info->help_string)
          set_tooltip(ret, ret->widget);
        }
      /* Spinbutton */
      else
        {
        ret->label = gtk_label_new(TR_DOM(info->real_name));
        gtk_misc_set_alignment(GTK_MISC(ret->label), 0.0, 0.5);
        ret->adjustment = gtk_adjustment_new(0.0,
                                             -1.0e8,
                                             1.0e8,
                                             1.0,
                                             1.0,
                                             1.0);
        ret->widget = gtk_spin_button_new(GTK_ADJUSTMENT(ret->adjustment),
                                          0.0,
                                          0);

        gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ret->widget),
                                   info->num_digits);

        if(info->help_string)
          set_tooltip(ret, ret->widget);
        }
      break;
    case LQT_PARAMETER_STRING:
      ret->label = gtk_label_new(TR_DOM(info->real_name));
      gtk_misc_set_alignment(GTK_MISC(ret->label), 0.0, 0.5);
      ret->widget = gtk_entry_new();
      if(info->help_string)
        set_tooltip(ret, ret->widget);
      break;
    case LQT_PARAMETER_STRINGLIST:    /* String with options */
      ret->selected = 0;
      ret->label = gtk_label_new(TR_DOM(info->real_name));
      gtk_misc_set_alignment(GTK_MISC(ret->label), 0.0, 0.5);

#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION >= 24))
      ret->widget = gtk_combo_box_text_new();
#else
      ret->widget = gtk_combo_box_new_text();
#endif
      for(i = 0; i < info->num_stringlist_options; i++)
        {
#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION >= 24))
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (ret->widget),
				   info->stringlist_options[i]);
#else
	gtk_combo_box_append_text (GTK_COMBO_BOX (ret->widget), 
				   info->stringlist_options[i]);
#endif
        }
      g_signal_connect(GTK_COMBO_BOX (ret->widget),
		       "changed",
		       G_CALLBACK(parameter_combobox_callback),
		       ret);

      if(info->help_string)
        g_signal_connect (ret->widget, "realize",
                          G_CALLBACK (realize_combo), ret);

      
      break;
    case LQT_PARAMETER_SECTION:
      break;
    }

  gtk_widget_set_size_request(ret->widget, 100, -1);
  
  parameter_widget_update(ret);

  gtk_widget_show(ret->widget);
  if(ret->label)
    gtk_widget_show(ret->label);
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

#if GTK_MINOR_VERSION < 12
static GtkWidget * create_table(lqt_parameter_info_t * parameter_info,
                                LqtGtkParameterWidget ** widgets,
                                int num_parameters, GtkTooltips * tooltips, const char * gettext_domain)
#else
static GtkWidget * create_table(lqt_parameter_info_t * parameter_info,
                                LqtGtkParameterWidget ** widgets,
                                int num_parameters, const char * gettext_domain)
#endif
  {
  int i;
  GtkWidget * ret;
  ret = gtk_table_new(num_parameters, 2, 0);
  gtk_container_set_border_width(GTK_CONTAINER(ret),
                                 10);
  gtk_table_set_row_spacings(GTK_TABLE(ret),
                             5);
  gtk_table_set_col_spacings(GTK_TABLE(ret),
                             5);
    
  for(i = 0; i < num_parameters; i++)
    {
#if GTK_MINOR_VERSION < 12
    widgets[i] =
      lqtgtk_create_parameter_widget(&parameter_info[i], tooltips, gettext_domain);
#else
    widgets[i] =
      lqtgtk_create_parameter_widget(&parameter_info[i], gettext_domain);
#endif
    /* Bool parameters have no labels */

    if(widgets[i]->label)
      {    
      gtk_table_attach(GTK_TABLE(ret), widgets[i]->label,
                       0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_attach(GTK_TABLE(ret), widgets[i]->widget,
                       1, 2, i, i+1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
      }
    else
      {
      gtk_table_attach(GTK_TABLE(ret), widgets[i]->widget,
                       0, 2, i, i+1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
      }
    }
  gtk_widget_show(ret);
  return ret;
  }

LqtGtkCodecConfigWidget *
lqtgtk_create_codec_config_widget(lqt_parameter_info_t * parameter_info,
                                  int num_parameters, const char * gettext_domain)
  {
  int i, parameter_index;
  GtkWidget * table;
  GtkWidget * notebook;
  GtkWidget * tab_label;
  int num_sections = 0;
  int parameters_in_section;
  LqtGtkCodecConfigWidget * ret = calloc(1, sizeof(LqtGtkCodecConfigWidget));

#if GTK_MINOR_VERSION < 12
  ret->tooltips = gtk_tooltips_new();
  g_object_ref (G_OBJECT (ret->tooltips));

#if GTK_MINOR_VERSION < 10
  gtk_object_sink (GTK_OBJECT (ret->tooltips));
#else
  g_object_ref_sink(G_OBJECT(ret->tooltips));
#endif
#endif
  
  if(parameter_info[0].type == LQT_PARAMETER_SECTION)
    {
    for(i = 0; i < num_parameters; i++)
      {
      if(parameter_info[i].type == LQT_PARAMETER_SECTION)
        num_sections++;
      }
    }
  
  ret->parameter_info = parameter_info;
  ret->num_parameters = num_parameters - num_sections;

  /* Create the parameter widgets */

  ret->parameter_widgets = calloc(num_parameters - num_sections,
                                  sizeof(LqtGtkParameterWidget *));

  if(num_sections)
    {
    parameter_index = 1;
    notebook = gtk_notebook_new();
    for(i = 0; i < num_sections; i++)
      {
      /* Count parameters in this section */

      parameters_in_section = 0;
      while((parameters_in_section + parameter_index < num_parameters) &&
            (parameter_info[parameters_in_section + parameter_index].type != LQT_PARAMETER_SECTION))
        {
        parameters_in_section++;
        }
      
      /* Create table */
#if GTK_MINOR_VERSION < 12
      table = create_table(&(parameter_info[parameter_index]),
                           &(ret->parameter_widgets[parameter_index - i - 1]),
                           parameters_in_section, ret->tooltips, gettext_domain);
#else
      table = create_table(&(parameter_info[parameter_index]),
                           &(ret->parameter_widgets[parameter_index - i - 1]),
                           parameters_in_section, gettext_domain);
#endif      
      /* Append table */

      tab_label = gtk_label_new(TR_DOM(parameter_info[parameter_index-1].real_name));
      gtk_widget_show(tab_label);
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, tab_label);
      
      /* Increment */

      parameter_index += parameters_in_section + 1;
      }
    gtk_widget_show(notebook);
    ret->widget = notebook;
    }
  else
#if GTK_MINOR_VERSION < 12
    ret->widget = create_table(parameter_info,
                               ret->parameter_widgets,
                               num_parameters, ret->tooltips,
                               gettext_domain);
#else
    ret->widget = create_table(parameter_info,
                               ret->parameter_widgets,
                               num_parameters,
                               gettext_domain);
#endif
  return ret;
  }

/* 
 * 
 */
#if 0
static void codec_config_widget_apply(LqtGtkCodecConfigWidget * ccw)
  {
  int i;
  for(i = 0; i < ccw->num_parameters; i++)
    parameter_widget_apply(ccw->parameter_widgets[i]);
  }
#endif

static void codec_config_widget_update(LqtGtkCodecConfigWidget * ccw)
  {
  int i;
  for(i = 0; i < ccw->num_parameters; i++)
    parameter_widget_update(ccw->parameter_widgets[i]);
  }

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

static void browser_select_row_callback(GtkTreeSelection *selection,
					gpointer user_data)
  {
    LqtGtkCodecBrowser * cb = (LqtGtkCodecBrowser *)user_data;
    GtkTreeModel * model;
    GtkTreeIter iter;
    gint row;

    if (gtk_tree_selection_get_selected(selection, &model, &iter) == TRUE)
      {

      if(cb->selected == -1)	/* ??? */
	gtk_widget_set_sensitive(cb->info_button, 1);

      gtk_tree_model_get(model, &iter, LQGTK_LIST_CODEC_INDEX_COLUMN_ID, &row, -1);
      cb->selected = row;
      cb->codec_info = cb->codecs[row];

      if((cb->encode && cb->codec_info->num_encoding_parameters) ||
	 (cb->decode && cb->codec_info->num_decoding_parameters))
	gtk_widget_set_sensitive(cb->parameters_button, 1);
      else
	gtk_widget_set_sensitive(cb->parameters_button, 0);
	
      if(cb->selected == 0)
	gtk_widget_set_sensitive(cb->up_button, 0);
      else
	gtk_widget_set_sensitive(cb->up_button, 1);
	
      if(cb->selected == cb->num_codecs - 1)
	gtk_widget_set_sensitive(cb->down_button, 0);
      else
	gtk_widget_set_sensitive(cb->down_button, 1);
      }
  }

static
void lqtgtk_codec_browser_model_update(GtkTreeModel * model,
				       int            num_codecs,
				       lqt_codec_info_t ** codecs)
  {
  int i;
  const char * gettext_domain;
  GtkListStore * store = GTK_LIST_STORE(model);
  GtkTreeIter iter;
  
  gtk_list_store_clear(store);
  for(i = 0; i < num_codecs; i++)
    {
    gtk_list_store_append(store, &iter);

    if(codecs[i]->gettext_domain && codecs[i]->gettext_directory)
      {
      bindtextdomain(codecs[i]->gettext_domain,
                     codecs[i]->gettext_directory);
      gettext_domain = codecs[i]->gettext_domain;
      }
    else
      {
      lqt_translation_init();
      gettext_domain = PACKAGE;
      }
    
    gtk_list_store_set(store, &iter, 
		       LQGTK_LIST_CODEC_NAME_COLUMN_ID, 
		       TR_DOM(codecs[i]->long_name),
		       LQGTK_LIST_CODEC_INDEX_COLUMN_ID, 
		       i,
		       -1);
    }

  }

static void browser_move_codec(LqtGtkCodecBrowser * cb, int pos, int new_pos)
  {
  lqt_codec_info_t * tmp_info;
  GtkTreePath * path;
  GtkTreeSelection * selection;

  tmp_info = cb->codecs[pos];
  cb->codecs[pos] = cb->codecs[new_pos];
  cb->codecs[new_pos] = tmp_info;

  /* We immediately apply this in the registry */

  if(cb->type == LQT_CODEC_AUDIO)
    lqt_reorder_audio_codecs(cb->codecs);
  else if(cb->type == LQT_CODEC_VIDEO)
    lqt_reorder_video_codecs(cb->codecs);
    
  /* Enable up button */ 
  
  if(!cb->selected && new_pos)
    gtk_widget_set_sensitive(cb->up_button, 1);

  /* Enable down button */

  if((cb->selected == cb->num_codecs - 1) && (new_pos < cb->num_codecs - 1))
    gtk_widget_set_sensitive(cb->down_button, 1);

  /* Disable up button */
 
  if(!new_pos)
    gtk_widget_set_sensitive(cb->up_button, 0);

  /* Disable down button */

  if(new_pos == cb->num_codecs - 1)
    gtk_widget_set_sensitive(cb->down_button, 0);
  
  cb->selected = new_pos;

  lqtgtk_codec_browser_model_update(cb->model, cb->num_codecs, cb->codecs);
  
  /* The selected codec should still be visible */
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(cb->list));
  path = gtk_tree_path_new_from_indices(cb->selected, -1);
  gtk_tree_selection_select_path(selection, path);
  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(cb->list), path, NULL, FALSE, 0.0, 0.0);
  gtk_tree_path_free(path);
  }

static void browser_button_callback(GtkWidget * w, gpointer data)
  {
  LqtGtkCodecBrowser * cb = (LqtGtkCodecBrowser *)data;

  LqtGtkCodecConfigWindow * codec_config_window;
  LqtGtkCodecInfoWindow * codec_info_window;
  
  if(w == cb->up_button)
    browser_move_codec(cb, cb->selected, cb->selected-1);
  else if(w == cb->down_button)
    browser_move_codec(cb, cb->selected, cb->selected+1);
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

LqtGtkCodecBrowser * lqtgtk_create_codec_browser(lqt_codec_type type,
                                                 int encode, int decode)
  {
  LqtGtkCodecBrowser * ret = calloc(1, sizeof(LqtGtkCodecBrowser));
  GtkCellRenderer    * renderer;
  GtkTreeSelection   * selection;
  
  ret->encode = encode;
  ret->decode = decode;
  ret->type   = type;
  
  ret->widget = gtk_table_new(4, 2, 0); 
  gtk_table_set_row_spacings(GTK_TABLE(ret->widget), 10);
  gtk_table_set_col_spacings(GTK_TABLE(ret->widget), 10);
  gtk_container_set_border_width(GTK_CONTAINER(ret->widget), 10);


  ret->model = (GtkTreeModel *)gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  ret->list  = gtk_tree_view_new_with_model(ret->model);
  renderer   = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ret->list),
					      -1,
					      _("Installed codecs"),
					      renderer,
					      "text", LQGTK_LIST_CODEC_NAME_COLUMN_ID,
					      NULL);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->list));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(browser_select_row_callback),
		   (gpointer)ret);
  
  gtk_widget_show(ret->list);
 
  ret->scrolledwindow =
    gtk_scrolled_window_new(NULL, NULL);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->scrolledwindow),
                                 GTK_POLICY_NEVER,
                                 GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(ret->scrolledwindow), ret->list);
  gtk_widget_show(ret->scrolledwindow);

  gtk_table_attach_defaults(GTK_TABLE(ret->widget), ret->scrolledwindow,
                            0, 1, 0, 4);
  
  ret->up_button =         gtk_button_new_with_label(_("Up"));
  ret->down_button =       gtk_button_new_with_label(_("Down"));

  ret->parameters_button = gtk_button_new_with_label(_("Parameters..."));
  ret->info_button =       gtk_button_new_with_label(_("Info..."));

  g_signal_connect(G_OBJECT(ret->up_button), "clicked",
		   G_CALLBACK(browser_button_callback),
		   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->down_button), "clicked",
		   G_CALLBACK(browser_button_callback),
		   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->parameters_button), "clicked",
		   G_CALLBACK(browser_button_callback),
		   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->info_button), "clicked",
		   G_CALLBACK(browser_button_callback),
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
  b->num_codecs = 0;
  
  if(b->codecs)
    lqt_destroy_codec_info(b->codecs);
    
  if(b->type == LQT_CODEC_AUDIO)
    b->codecs = lqt_query_registry(1, 0, b->encode, b->decode);
  else
    b->codecs = lqt_query_registry(0, 1, b->encode, b->decode);

  while(1)
    {
    if(b->codecs[b->num_codecs])
      b->num_codecs++;
    else
      break;
    }

  lqtgtk_codec_browser_model_update(b->model,
				    b->num_codecs,
				    b->codecs);

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
  int encode, decode;
  
  LqtGtkCodecConfigWindow * ccw = (LqtGtkCodecConfigWindow *)data;
  if(w == ccw->apply_button)
    {
    lqtgtk_codec_config_window_apply(ccw);
    }
  else if(w == ccw->close_button)
    {
    gtk_widget_hide(ccw->window);
    gtk_main_quit();
    }
  else if(w == ccw->restore_button)
    {
    encode = (ccw->encode_widget) ? 1 : 0;
    decode = (ccw->decode_widget) ? 1 : 0;

    /*    fprintf(stderr, "Restoring parameters %d %d\n", encode, decode); */
    
    lqt_restore_default_parameters(ccw->codec_info, encode, decode);

    if(encode)
      {
      /*      fprintf(stderr, "Updating encoding parameters\n"); */
      codec_config_widget_update(ccw->encode_widget);
      }
    if(decode)
      {
      /*      fprintf(stderr, "Updating decoding parameters\n"); */
      codec_config_widget_update(ccw->decode_widget);
      }
      
    
    }
  }

LqtGtkCodecConfigWindow *
lqtgtk_create_codec_config_window(lqt_codec_info_t * codec_info,
                                  int encode,
                                  int decode)
  {
  const char * gettext_domain;

  LqtGtkCodecConfigWindow * ret =
    calloc(1, sizeof(LqtGtkCodecConfigWindow));

  ret->codec_info = codec_info;

  if(codec_info->gettext_domain && codec_info->gettext_directory)
    {
    bindtextdomain(codec_info->gettext_domain,
                   codec_info->gettext_directory);
    gettext_domain = codec_info->gettext_domain;
    }
  else
    {
    lqt_translation_init();
    gettext_domain = PACKAGE;
    }
  
  
  if(encode && codec_info->num_encoding_parameters)
    ret->encode_widget =
      lqtgtk_create_codec_config_widget(codec_info->encoding_parameters,
                                        codec_info->num_encoding_parameters, gettext_domain);

  if(decode && codec_info->num_decoding_parameters)
    ret->decode_widget =
      lqtgtk_create_codec_config_widget(codec_info->decoding_parameters,
                                        codec_info->num_decoding_parameters, gettext_domain);

  if(encode && decode)
    {
    ret->hbox = gtk_hbox_new(0, 5);
    if(codec_info->num_encoding_parameters)
      {
      ret->encoding_frame = gtk_frame_new(_("Encoding Options"));
      
      gtk_container_add(GTK_CONTAINER(ret->encoding_frame),
                       ret->encode_widget->widget);
      gtk_widget_show(ret->encoding_frame);
      gtk_box_pack_start(GTK_BOX(ret->hbox), ret->encoding_frame, TRUE, TRUE, 0);
      }
    if(codec_info->num_decoding_parameters)
      {
      ret->decoding_frame = gtk_frame_new(_("Decoding Options"));
      gtk_container_add(GTK_CONTAINER(ret->decoding_frame),
                        ret->decode_widget->widget);
      gtk_widget_show(ret->decoding_frame);
      gtk_box_pack_start(GTK_BOX(ret->hbox), ret->decoding_frame, TRUE, TRUE, 0);
      }
    gtk_widget_show(ret->hbox);
    }
  
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(ret->window), codec_info->long_name);
  gtk_window_set_modal(GTK_WINDOW(ret->window), TRUE);
  
  ret->apply_button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  ret->restore_button = gtk_button_new_with_label(_("Restore defaults"));

  g_signal_connect(G_OBJECT(ret->apply_button),
		   "clicked",
		   G_CALLBACK(codec_config_window_button_callback),
		   (gpointer)ret);
  
  g_signal_connect(G_OBJECT(ret->close_button),
		   "clicked",
		   G_CALLBACK(codec_config_window_button_callback),
		   (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->restore_button),
		   "clicked",
		   G_CALLBACK(codec_config_window_button_callback),
		   (gpointer)ret);

  lqtgtk_widget_set_can_default(ret->apply_button, TRUE);
  lqtgtk_widget_set_can_default(ret->close_button, TRUE);
  lqtgtk_widget_set_can_default(ret->restore_button, TRUE);
  
  gtk_widget_show(ret->apply_button);
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->restore_button);

  ret->buttonbox = gtk_hbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(ret->buttonbox), 5);

  ret->mainbox = gtk_vbox_new(0, 5);
  gtk_container_set_border_width(GTK_CONTAINER(ret->mainbox), 10);

  if(encode && decode)
    gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->hbox, TRUE, TRUE, 0);
  else if(encode)
    gtk_box_pack_start(GTK_BOX(ret->mainbox),
                                ret->encode_widget->widget, TRUE, TRUE, 0);
  else if(decode)
    gtk_box_pack_start(GTK_BOX(ret->mainbox),
                                ret->decode_widget->widget, TRUE, TRUE, 0);
    
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->close_button);
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->restore_button);

  gtk_widget_show(ret->buttonbox);
  gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->buttonbox, TRUE, TRUE, 0);
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
  int i, index;

  /*
   *  First, all parameter widgets transfer their values to the
   *  lqt_parameter_t structures
   */

  if(w->encode_widget)
    {
    index = 0;
    for(i = 0; i < w->codec_info->num_encoding_parameters; i++)
      {
      if(w->codec_info->encoding_parameters[i].type == LQT_PARAMETER_SECTION)
        continue;
      parameter_widget_apply(w->encode_widget->parameter_widgets[index]);
      lqt_set_default_parameter(w->codec_info->type,
                                1, w->codec_info->name,
                                w->codec_info->encoding_parameters[i].name,
                                &(w->encode_widget->parameter_info[i].val_default));
      index++;
      }
    }
  if(w->decode_widget)
    {
    index = 0;
    for(i = 0; i < w->codec_info->num_decoding_parameters; i++)
      {
      if(w->codec_info->decoding_parameters[i].type == LQT_PARAMETER_SECTION)
        continue;
      parameter_widget_apply(w->decode_widget->parameter_widgets[index]);
      lqt_set_default_parameter(w->codec_info->type,
                                0, w->codec_info->name,
                                w->codec_info->decoding_parameters[i].name,
                                &(w->decode_widget->parameter_info[i].val_default));
      index++;
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

  const char * gettext_domain;
  LqtGtkCodecInfoWidget * ret;

  if(info->gettext_domain && info->gettext_directory)
    {
    bindtextdomain(info->gettext_domain,
                   info->gettext_directory);
    gettext_domain = info->gettext_domain;
    }
  else
    {
    lqt_translation_init();
    gettext_domain = PACKAGE;
    }
  ret = calloc(1, sizeof(LqtGtkCodecInfoWidget));  

  ret->real_name = gtk_label_new(TR_DOM(info->long_name));
  gtk_misc_set_alignment(GTK_MISC(ret->real_name), 0.0, 0.5);
  ret->short_name_label = gtk_label_new(_("Internal name: "));
  gtk_misc_set_alignment(GTK_MISC(ret->short_name_label), 0.0, 0.5);
  ret->short_name = gtk_label_new(TR_DOM(info->name));
  gtk_misc_set_alignment(GTK_MISC(ret->short_name), 0.0, 0.5);
  ret->description = gtk_label_new(TR_DOM(info->description));
  gtk_misc_set_alignment(GTK_MISC(ret->description), 0.0, 0.5);
  
  ret->module_filename = gtk_label_new(info->module_filename);
  gtk_misc_set_alignment(GTK_MISC(ret->module_filename), 0.0, 0.5);
  ret->module_filename_label = gtk_label_new(_("Module: "));
  gtk_misc_set_alignment(GTK_MISC(ret->module_filename_label), 0.0, 0.5);
  
  gtk_widget_show(ret->real_name);
  gtk_widget_show(ret->short_name);
  gtk_widget_show(ret->short_name_label);
  gtk_widget_show(ret->description);

  gtk_widget_show(ret->module_filename);
  gtk_widget_show(ret->module_filename_label);
    
  /* Create the fourccs label */

  if(info->num_fourccs)
    {
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
    }
  
  /* Create wav_ids label */

  if(info->num_wav_ids)
    {
    tmp1 = malloc(info->num_wav_ids * 30);
    tmp2 = malloc(30);
    
    *tmp1 = '\0';
    
    for(i = 0; i < info->num_wav_ids - 1; i++)
      {
      sprintf(tmp2, "0x%02x,", info->wav_ids[i]);
      strcat(tmp1, tmp2);
      }
    
    /* Last one without comma */
    
    sprintf(tmp2, "0x%02x", info->wav_ids[i]);
    strcat(tmp1, tmp2);
    
    ret->wav_ids_label = gtk_label_new(tmp1);
    gtk_widget_show(ret->wav_ids_label);
    
    free(tmp1);
    free(tmp2);
    }
    
  /* Create encoding colormodels label */

  
  /* Pack all widgets onto their containers */

  if(ret->fourccs_label)
    {
    ret->fourccs_frame = gtk_frame_new(_("Fourccs"));
    
    gtk_container_add(GTK_CONTAINER(ret->fourccs_frame), ret->fourccs_label);
    gtk_widget_show(ret->fourccs_frame);
    }
  
  if(ret->wav_ids_label)
    {
    ret->wav_ids_frame = gtk_frame_new(_("WAV Id(s)"));
    
    gtk_container_add(GTK_CONTAINER(ret->wav_ids_frame), ret->wav_ids_label);
    gtk_widget_show(ret->wav_ids_frame);
    }
  
  ret->label_table = gtk_table_new(6, 2, 0);

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

  ret->table = gtk_table_new(3, 2, 0);

  gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->label_table,
                            0, 2, 0, 1);

  if(info->type == LQT_CODEC_VIDEO)
    {
    if(ret->fourccs_frame)
      {
      gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->fourccs_frame,
                                0, 2, 1, 2);
      }
    }
  else if(ret->wav_ids_frame)
    {
    if(ret->fourccs_frame)
      {
      gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->fourccs_frame,
                                0, 1, 1, 2);
      gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->wav_ids_frame,
                                1, 2, 1, 2);
      }
    else
      gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->wav_ids_frame,
                                0, 2, 1, 2);
    }
  
  else if(ret->fourccs_frame)
    gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->fourccs_frame,
                              0, 2, 1, 2);
  gtk_widget_show(ret->table);
  ret->widget = ret->table;
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
  gtk_window_set_modal(GTK_WINDOW(ret->window), TRUE);

  ret->mainbox = gtk_vbox_new(0, 10);

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  lqtgtk_widget_set_can_default(ret->close_button, TRUE);
  
  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                     G_CALLBACK(codec_info_window_button_callback),
                     (gpointer)ret);
    
  gtk_widget_show(ret->close_button);
  
  gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->info_widget->widget, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->close_button, TRUE, TRUE, 0);
  
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

#if 0


/*
 *   LqtGtkEncoderWidget
 */

static void
encoder_widget_button_callback(GtkWidget * w, gpointer data)
  {
  LqtGtkEncoderWidget * ew = (LqtGtkEncoderWidget*)data;
  LqtGtkCodecInfoWindow * iw;
  LqtGtkCodecConfigWindow * cw;
  if(w == ew->info_button)
    {
    iw = lqtgtk_create_codec_info_window(ew->encoders[ew->selected]);
    lqtgtk_codec_info_window_run(iw);
    }
  else if(w == ew->parameters_button)
    {
    cw = lqtgtk_create_codec_config_window(ew->encoders[ew->selected], 1, 0);
    lqtgtk_codec_config_window_run(cw);
    }
  }

LqtGtkEncoderWidget *
lqtgtk_create_encoder_widget(lqt_codec_type type)
  {
  LqtGtkEncoderWidget * ret = calloc(1, sizeof(LqtGtkEncoderWidget));
  ret->type = type;
  ret->info_button = gtk_button_new_with_label(_("Info..."));
  ret->parameters_button = gtk_button_new_with_label(_("Parameters..."));

  g_signal_connect(G_OBJECT(ret->info_button), "clicked",
		   G_CALLBACK(encoder_widget_button_callback),
		   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->parameters_button), "clicked",
		   G_CALLBACK(encoder_widget_button_callback),
		   (gpointer)ret);
    
  gtk_widget_show(ret->info_button);
  gtk_widget_show(ret->parameters_button);

  ret->optionmenu = gtk_option_menu_new();
  
  ret->menu = gtk_menu_new();
  gtk_widget_show(ret->menu);
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(ret->optionmenu), ret->menu);
  gtk_widget_show(ret->optionmenu);
  
  /* NEED CHECK AGAIN */
  /* gtk_widget_set_usize(ret->optionmenu, 200, ret->optionmenu->requisition.height); */
  gtk_widget_set_size_request(ret->optionmenu, 200, -1);
  
  return ret;
  }

void lqtgtk_destroy_encoder_widget(LqtGtkEncoderWidget * ew)
  {
  if(ew->encoders)
    lqt_destroy_codec_info(ew->encoders);
  }

static void encoder_widget_update_selected(LqtGtkEncoderWidget * ew)
  {
  gtk_option_menu_set_history(GTK_OPTION_MENU(ew->optionmenu), ew->selected);
  ew->current_encoder = ew->encoders[ew->selected];
  if(ew->current_encoder->num_encoding_parameters)
    gtk_widget_set_sensitive(ew->parameters_button, 1);
  else
    gtk_widget_set_sensitive(ew->parameters_button, 0);
  }

static void encoder_widget_menu_callback(GtkWidget * w, gpointer data)
  {
  int i;
  
  LqtGtkEncoderWidget * ew = (LqtGtkEncoderWidget*)data;
  
  for(i = 0; i < ew->num_encoders; i++)
    {
    if(w == ew->menu_items[i])
      {
      ew->selected = i;
      encoder_widget_update_selected(ew);
      }
    }
  
  }

void lqtgtk_encoder_widget_update(LqtGtkEncoderWidget * ew)
  {
  GtkWidget ** new_items;
  int i;
  char * label_text;
  
  if(ew->encoders)
    lqt_destroy_codec_info(ew->encoders);
    
  if(ew->type == LQT_CODEC_AUDIO)
    ew->encoders = lqt_query_registry(1, 0, 1, 0);
  else
    ew->encoders = lqt_query_registry(0, 1, 1, 0);

  /* Count the codecs */
  ew->num_encoders = 0;
    
  while(1)
    {
    if(!(ew->encoders[ew->num_encoders]))
      break;
    else
      ew->num_encoders++;
    }

  /* Create new menu items */
  
  if(ew->num_menu_items < ew->num_encoders)
    {
    new_items = calloc(1, ew->num_encoders * sizeof(GtkWidget*));

    /* Copy old items */

    if(ew->menu_items)
      {
      for(i = 0; i < ew->num_menu_items; i++)
        new_items[i] = ew->menu_items[i];
      free(ew->menu_items);
      }
    ew->menu_items = new_items;
        
    /* Create new items */
    
    for(i = ew->num_menu_items; i < ew->num_encoders; i++)
      {
      ew->menu_items[i] = gtk_menu_item_new_with_label(" ");
      g_signal_connect(G_OBJECT(ew->menu_items[i]),
		       "activate",
		       G_CALLBACK(encoder_widget_menu_callback),
		       (gpointer)ew);
      gtk_menu_shell_append(GTK_MENU_SHELL(ew->menu), ew->menu_items[i]);
      }
    
    ew->num_menu_items = ew->num_encoders;
    }

  /* Update menu labels and show all active items */

  for(i = 0; i < ew->num_encoders; i++)
    {
    label_text = g_strdup_printf("%s (%s)",
                          ew->encoders[i]->long_name,
                          ew->encoders[i]->name);
    if(GTK_BIN(ew->menu_items[i])->child)
      {
      gtk_label_set_text(GTK_LABEL(GTK_BIN(ew->menu_items[i])->child),
                         label_text);
      }
    else
      {
	gtk_button_set_label(GTK_BUTTON(ew->optionmenu), label_text);
      }
    g_free(label_text);
    gtk_widget_show(ew->menu_items[i]);
    }

  /* Hide other items */

  for(i = ew->num_encoders; i < ew->num_menu_items; i++)
    gtk_widget_hide(ew->menu_items[i]);

  if(ew->selected >= ew->num_encoders)
    ew->selected = 0;
  encoder_widget_update_selected(ew);
  }

const char * lqtgtk_encoder_widget_get_encoder(LqtGtkEncoderWidget * ew)
  {
  return ew->current_encoder->name;
  }

void lqtgtk_encoder_widget_set_encoder(LqtGtkEncoderWidget * ew,
                                       const char * name)
  {
  int i;
  ew->selected = 0;

  for(i = 0; i < ew->num_encoders; i++)
    {
    if(!strcmp(ew->encoders[i]->name, name))
      {
      ew->selected = i;
      break;
      }
    }
  encoder_widget_update_selected(ew);
  }

#endif
