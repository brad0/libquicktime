#include "lqt_private.h"
#include "lqt_gtk.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <locale.h>

#include <libintl.h>
#define _(str) dgettext(PACKAGE, str)

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
  //  GtkWidget * rescan_button;

  GtkWidget * notebook;

  GtkWidget * buttonbox;

  GtkWidget * mainbox;
    
  } MainWindow;

static void update_main_window(MainWindow * w)
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
  }

static gboolean delete_callback(GtkWidget * w, gpointer data)
  {
  gtk_main_quit();
  return TRUE;
  }

static MainWindow * create_main_window()
  {
  GtkWidget * tab_label;
  
  MainWindow * ret = calloc(1, sizeof(MainWindow));
    
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), "Libquicktime configurator "VERSION);

  //  gtk_widget_set_size_request(ret->window, 350, 200);
  
  g_signal_connect(G_OBJECT(ret->window), "delete-event",
                   G_CALLBACK(delete_callback), ret);
    

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  ret->save_button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
  
  
  ret->notebook = gtk_notebook_new();

  tab_label = gtk_label_new(_("Audio Codecs"));
  gtk_widget_show(tab_label);

  ret->audio_browser = lqtgtk_create_codec_browser(LQT_CODEC_AUDIO, 1, 1);
    
  gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                           ret->audio_browser->widget, tab_label);
      
  tab_label = gtk_label_new(_("Video Codecs"));
  gtk_widget_show(tab_label);
  
  ret->video_browser = lqtgtk_create_codec_browser(LQT_CODEC_VIDEO, 1, 1);
    
  gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                           ret->video_browser->widget, tab_label);

  gtk_widget_show(ret->notebook);
    
  ret->buttonbox = gtk_hbutton_box_new();
   

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  ret->save_button = gtk_button_new_from_stock(GTK_STOCK_SAVE);

  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(main_window_button_callback),
                   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->save_button), "clicked",
                   G_CALLBACK(main_window_button_callback),
                   (gpointer)ret);

  lqtgtk_widget_set_can_default(ret->close_button, TRUE);
  lqtgtk_widget_set_can_default(ret->save_button, TRUE);
  
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->save_button);

  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->close_button);
  gtk_container_add(GTK_CONTAINER(ret->buttonbox), ret->save_button);

  gtk_widget_show(ret->buttonbox);

  ret->mainbox = gtk_vbox_new(0, 5);

  gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->notebook, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ret->mainbox), ret->buttonbox,
                     FALSE, FALSE, 0);

  gtk_widget_show(ret->mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->mainbox);
    
  return ret;
    
  }

static void destroy_main_window(MainWindow * w)
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

  bindtextdomain(PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  
  /* No, we don't like commas as decimal separators */
  setlocale(LC_NUMERIC, "C");

  main_window = create_main_window();
  update_main_window(main_window);
  
  gtk_widget_show(main_window->window);

  gtk_main();
    
  destroy_main_window(main_window);
  return 0;
  }
