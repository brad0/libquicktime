#include <quicktime/lqt.h>

#include <gtk/gtk.h>

#include "lqt_gtk.h"

typedef struct
  {
  
  
  GtkWidget * window;

  GtkWidget * close_button;
  GtkWidget * apply_button;
  
  } CodecConfigWindow;

typedef struct 
  {
  LqtGtkCodecBrowser * audio_browser;
  LqtGtkCodecBrowser * video_browser;
  
  GtkWidget * window;

  GtkWidget * close_button;
  GtkWidget * save_button;
  GtkWidget * rescan_button;

  GtkWidget * notebook;

  GtkWidget * buttonbox;

  GtkWidget * mainbox;
    
  } MainWindow;

void update_main_window(MainWindow * w)
  {
  lqtgtk_codec_browser_update(w->audio_browser);
  lqtgtk_codec_browser_update(w->video_browser);
  }

static void main_window_button_callback(GtkWidget * w, gpointer data)
  {
  MainWindow * mw = (MainWindow *)data;
  if(w == mw->close_button)
    gtk_main_quit();
  else if(w == mw->save_button)
    {
    lqt_registry_write();
    }
  else if(w == mw->rescan_button)
    {
    
    }
  }

MainWindow * create_main_window()
  {
  GtkWidget * tab_label;
  
  MainWindow * ret = calloc(1, sizeof(MainWindow));
    
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  ret->close_button = gtk_button_new_with_label("Close");
  ret->save_button = gtk_button_new_with_label("Save");
  ret->rescan_button = gtk_button_new_with_label("Scan for codecs");

  ret->notebook = gtk_notebook_new();

  tab_label = gtk_label_new("Audio Codecs");
  gtk_widget_show(tab_label);

  ret->audio_browser = lqtgtk_create_codec_browser(LQT_CODEC_AUDIO, 1, 1);
    
  gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                           ret->audio_browser->widget, tab_label);
      
  tab_label = gtk_label_new("Video Codecs");
  gtk_widget_show(tab_label);
  
  ret->video_browser = lqtgtk_create_codec_browser(LQT_CODEC_VIDEO, 1, 1);
    
  gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                           ret->video_browser->widget, tab_label);

  gtk_widget_show(ret->notebook);
    
  ret->buttonbox = gtk_hbutton_box_new();
   

  ret->close_button = gtk_button_new_with_label("Close");
  ret->save_button = gtk_button_new_with_label("Save");
  ret->rescan_button = gtk_button_new_with_label("Scan for codecs");

  gtk_signal_connect(GTK_OBJECT(ret->close_button), "clicked",
                     GTK_SIGNAL_FUNC(main_window_button_callback),
                     (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->save_button), "clicked",
                     GTK_SIGNAL_FUNC(main_window_button_callback),
                     (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->rescan_button), "clicked",
                     GTK_SIGNAL_FUNC(main_window_button_callback),
                     (gpointer)ret);

  GTK_WIDGET_SET_FLAGS (ret->close_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (ret->save_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (ret->rescan_button, GTK_CAN_DEFAULT);
  
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->save_button);
  gtk_widget_show(ret->rescan_button);

  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->close_button);
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->save_button);
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->rescan_button);

  gtk_widget_show(ret->buttonbox);

  ret->mainbox = gtk_vbox_new(0, 5);

  gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox), ret->notebook);
  gtk_box_pack_start_defaults(GTK_BOX(ret->mainbox), ret->buttonbox);

  gtk_widget_show(ret->mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->mainbox);
    
  return ret;
    
  }

void destroy_main_window(MainWindow * w)
  {
  lqtgtk_destroy_codec_browser(w->audio_browser);
  lqtgtk_destroy_codec_browser(w->video_browser);

  gtk_widget_destroy(w->window);
  free(w);
  }

int main(int argc, char ** argv)
  {
  MainWindow * main_window;

  gtk_init(&argc, &argv);
  
  main_window = create_main_window();
  update_main_window(main_window);
  
  gtk_widget_show(main_window->window);

  gtk_main();
    
  destroy_main_window(main_window);
  return 0;
  }
