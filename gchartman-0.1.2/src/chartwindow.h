#ifndef __CHARTWINDOW_H__
#define __CHARTWINDOW_H__

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <libxml/tree.h>
#include "connectdb.h"
#include "module.h"

GnomeDialog	*get_config_name_dialog;
GtkWidget	*new_config_pos_button;
GnomeEntry	*new_config_name_entry;
GnomeDialog	*edit_comment_dialog;
GtkText		*comment_textbox;
GnomeAppBar	*chart_appbar;

GnomeDialog	*split_stock_dialog;
GtkSpinButton	*split_stock_upper_spin;
GtkSpinButton	*split_stock_lower_spin;
GnomeDateEdit	*split_stock_date_edit;

GtkCombo	*global_config_name_combo;
GtkCombo	*local_config_name_combo;

GtkCombo	*chart_cat_combo;
GtkCombo	*chart_stock_combo;

GtkButton	*instance_delete_button;
GtkButton	*instance_properties_button;
GtkCTreeNode	*selected_instance_node;

gint on_chartcanvas_root_event(GnomeCanvasItem *item, GdkEvent *event, gpointer data);
GtkScrolledWindow *chart_scrolled_window;
GtkWidget	*chart_window;

GtkToolbar	*chart_type_toolbar;

Quote	*quote_vector;
int	quote_count;		//Number of quotes in QuoteV

GList	*type_list;

void create_from_toolbar(GtkWidget *widget, gpointer user_data);
ChartInstance *add_chart_instance(ChartType *ct,GtkCTreeNode *parent,xmlNodePtr config_node,Quote *quote_vector,int quote_count);
void reorder_y_instances(GtkCTree *tree);
void update_global_conf_combo(void);
void update_local_conf_combo(void);
void adjust_scroll_region(void);
//GList	*instance_list;

#endif
