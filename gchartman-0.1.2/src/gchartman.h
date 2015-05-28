#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include <mysql/mysql.h>
#include <libxml/tree.h>
#include <libxml/entities.h>

#define	GLADE_FILE PLUGINDIR"/gchartman.glade"

/*
 *	Global Widget Objects
 */

char	global_configs_filename[512];
char	local_configs_filename[512];
xmlDocPtr	global_configs_doc;
xmlDocPtr	local_configs_doc;
xmlNodePtr	db_global_configs_node;
xmlNodePtr	db_local_configs_node;

GHashTable	*name_cache;	// key=tname, value=name
GHashTable	*tname_cache;	// key=name, value=tname

GtkCTreeNode	*selected_node;
GtkLabel	*database_status_label;
GtkCTree	*table_tree;
GtkCTree	*chart_structure_ctree;
GnomeDialog	*trading_time_dialog;
GladeXML *xml;
GtkCList	*tt_days_clist;
GtkSpinButton	*tt_offset_spinbutton;
GtkSpinButton	*tt_hstart_spinbutton;
GtkSpinButton	*tt_hend_spinbutton;
GtkSpinButton	*tt_mstart_spinbutton;
GtkSpinButton	*tt_mend_spinbutton;
GtkButton	*trading_time_button;
GtkCheckButton	*tt_trading_checkbutton;
GtkButton	*disconnect_button;
GtkButton	*connect_button;
GtkButton	*add_category_button;
GtkButton	*delete_stock_button;
GtkButton	*view_chart_button;

typedef struct {	// Per day
	int	hstart;
	int	mstart;
	int	hend;
	int	mend;
	int	offset;
	int	trading;
} Day_TradeTime;

typedef struct {
	Day_TradeTime ttime[7];
} TradeTime;

/*
 *	Functions exported by gchartman.c
 */

void	get_trading_times(TradeTime *t_time,const char *name);
void	set_wait_cursor(GtkWidget *widget, int toggle);
void	error_dialog(const char *Message);
int	ok_cancel_dialog(const char *message);
void	update_tablelist(MYSQL *mysql);
void	set_stock_names(const char *tname,const char *name);
char	*get_stock_name(const char *tname);
char	*get_stock_tname(const char *name,int *error);
char	*get_category_name(const char *tname);
char	*get_category_tname(const char *name);

