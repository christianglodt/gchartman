#include <gtk/gtk.h>
#include <gtk/gtkclist.h>
#include <glade/glade.h>
#include <gnome.h>
#include <glob.h>
#include <dlfcn.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "about.h"
#include "getvtquotes.h"
#include "connectdb.h"
#include "gchartman.h"
#include "chartwindow.h"
#include "module.h"

GtkCTreeNode *all_stocks_ctree_node;
GnomeDialog *add_category_dialog;
GnomeEntry *add_category_name;

void set_wait_cursor(GtkWidget *widget, int toggle) {
	static GdkCursor *cursor;
	
	assert(widget!=NULL);
	
	if (cursor!=NULL) gdk_cursor_destroy(cursor);

	gtk_widget_realize(widget);

	if (toggle==TRUE) {
		cursor=gdk_cursor_new(GDK_WATCH);
		gdk_window_set_cursor(widget->window,cursor);
	} else {
		cursor=gdk_cursor_new(GDK_LEFT_PTR);
		gdk_window_set_cursor(widget->window,cursor);
	}
}

int tempint;
GtkCTreeNode *node_found;

ChartType *make_chart_type(char *dlname) {
	char	*error;
	ChartType *ct;

	g_assert(dlname!=NULL);

	ct=(ChartType *)g_malloc(sizeof(ChartType));
	ct->handle=dlopen(dlname,RTLD_NOW|RTLD_GLOBAL);

	if (ct->handle==NULL) {
		error=dlerror();
		g_message(error);
		return NULL;
	}

	error=NULL;
	
	ct->create=dlsym(ct->handle,"create"); error=dlerror();
	if (error!=NULL) {
		g_message(error);
		return NULL;
	}

	ct->free_object=dlsym(ct->handle,"free_object"); error=dlerror();
	if (error!=NULL) {
		g_message(error);
		return NULL;
	}

	ct->set_properties=dlsym(ct->handle,"set_properties"); error=dlerror();
	if (error!=NULL) {
		ct->set_properties=NULL;
	}

        ct->get_caps=dlsym(ct->handle,"get_caps"); error=dlerror();
        if (error!=NULL) {
                g_message(error);
		return NULL;
        }

        ct->caps=(ct->get_caps());
        
	ct->update=dlsym(ct->handle,"update"); error=dlerror();
	if (error!=NULL) {
		g_message(error);
		return NULL;
	}

	ct->redraw=dlsym(ct->handle,"redraw"); error=dlerror();
	if (error!=NULL) {
		g_message(error);
		return NULL;
	}

	return ct;
}

void init_modules() {
	ChartType *ct;
	char	*item_name;
	char	globpattern[200];
	glob_t	pglob;
	int	i;
	GtkWidget * iconw;
	char	fn[200];

	type_list=NULL;

//�-�-�-�-�-�-�-�- - - - - - - - - - - - - - - - - - -
//	We need a loop that finds the chart modules and puts their
//	symbol addresses into a list of structures.

	sprintf(globpattern,"%s/lib*.so",PLUGINDIR);

	glob(globpattern,0,NULL,&pglob);

	gtk_widget_realize(chart_window);	//Needed for the gdk_pixmap_create_from_xpm_d

	i=0;
	while (pglob.gl_pathv[i]!=NULL) {
		ct=make_chart_type(pglob.gl_pathv[i]);

		if (ct) {
			type_list=g_list_append(type_list,ct);

			item_name=ct->caps->name;

			if (ct->caps->in_toolbar) {
				if (ct->caps->icon==NULL) iconw=gnome_pixmap_new_from_file(PLUGINDIR"/gtk.png");
				else {
					sprintf(fn,"%s/%s",PLUGINDIR,ct->caps->icon);
					iconw=gnome_pixmap_new_from_file(fn);
				}

				gtk_toolbar_append_item (chart_type_toolbar, /* our toolbar */
	                                   NULL,		/* button label */
        	                           item_name,		/* this button's tooltip */
                	                   "Private",		/* tooltip private info */
                        	           iconw,		/* icon widget */
                                	   create_from_toolbar,
	                                   ct);
			}
		}
		
		i++;
	}
	
	globfree(&pglob);
}

void FindFunc(GtkCTree     *ctree,
              GtkCTreeNode *node,
              gpointer      data) {

	if (strcmp(GTK_CELL_TEXT(GTK_CTREE_ROW(node)->row.cell[tempint])->text,data)==0) {
		node_found=node;
	}


}

GtkCTreeNode *find_node_by_text(GtkCTree *ctree,
				GtkCTreeNode *node,
				char *text,
				int col) {

	assert(ctree!=NULL);
	assert(node!=NULL);

	tempint=col;
	node_found=NULL;

	gtk_ctree_pre_recursive(ctree,		//GtkCTree     *ctree,
                                 node,		//GtkCTreeNode *node,
                                 FindFunc,	//GtkCTreeFunc  func,
                                 text);		//gpointer      data);
	return node_found;

}

void on_tabletree_move(GtkCTree     *ctree,
		       GtkCTreeNode *node,
                       GtkCTreeNode *new_parent,
		       GtkCTreeNode *new_sibling) {

	char	*parent_name;
	char	*parent_tname;
	char	*node_name;
	char	*node_tname;
	char	*new_parent_name;
	char	*new_parent_tname;
	char	dbquery[200];
	char	*list_data[4];
	GtkCTreeNode *temp_node;

	if (GTK_CTREE_ROW(node)->parent==new_parent) {
		//Do nothing in this case
		//g_message("Parent didn't change\n");
	} else {
		// 1.Find out parent's Tablename
		// 2.Find out new_parent's Tablename
		// 3.Find out node's Tablename
		// 4.Check if the node is already in the new_parent's table
		// 5.Delete Node from parent's table
		// 6.Insert it into new_parent table

	
//		gtk_ctree_node_get_text(ctree,GTK_CTREE_ROW(node)->parent,0,&parent_name);
	/*
	 *	Time for another evil kludge: Since gtk_ctree_node_get_text won't work,
	 *	we need to crawl through gtk internals to fetch it ourselves
	 */
		parent_name=GTK_CELL_TEXT(GTK_CTREE_ROW(GTK_CTREE_ROW(node)->parent)->row.cell[0])->text;
		parent_tname=get_category_tname(parent_name);
//		g_message("Parent: %s, aka %s\n",parent_name,parent_tname);
		
		new_parent_name=GTK_CELL_TEXT(GTK_CTREE_ROW(new_parent)->row.cell[0])->text;
		new_parent_tname=get_category_tname(new_parent_name);
//		g_message("New Parent: %s, aka %s\n",new_parent_name,new_parent_tname);

		node_name=GTK_CELL_TEXT(GTK_CTREE_ROW(node)->row.cell[0])->text;
		node_tname=get_stock_tname(node_name,NULL);
//		g_message("Node: %s, aka %s\n",node_name,node_tname);

		list_data[0]=node_name;
		list_data[1]=GTK_CELL_TEXT(GTK_CTREE_ROW(node)->row.cell[1])->text;
		list_data[2]=GTK_CELL_TEXT(GTK_CTREE_ROW(node)->row.cell[2])->text;
		list_data[3]=NULL;

		//If we dragged from "All Stocks" don't do anything
		if (GTK_CTREE_ROW(node)->parent!=all_stocks_ctree_node) {
			sprintf(dbquery,"delete from %s where StockTableName=\'%s\'",parent_tname,node_tname);
			mysql_query(mysql,dbquery);
		} else {	//We need to re-insert the dragged node into "All Stocks"
			gtk_ctree_insert_node(table_tree,
						all_stocks_ctree_node,	//parent
						NULL,		//sibling
						&list_data[0],	//text
						5,		//spacing
						NULL,
						NULL,
						NULL,
						NULL,
						TRUE,		//isleaf
						FALSE);		//expanded
		}

		if ((temp_node=find_node_by_text(ctree,new_parent,node_name,0))!=NULL) {
			// This will never be called. Actually it used to be called,
			// but it crashed. Probably a re-entrancy problem in gtk.
			// Worked around it by moving  dupe-checking to CompareDragFunc.
			gtk_ctree_remove_node(ctree,temp_node);
		} else {
			sprintf(dbquery,"insert into %s (StockTableName) Values(\'%s\')",new_parent_tname,node_tname);
			mysql_query(mysql,dbquery);
		}

		gtk_ctree_sort_node(ctree,new_parent);
		gtk_ctree_sort_node(ctree,GTK_CTREE_ROW(node)->parent);
	}
}

gboolean compare_drag_func (GtkCTree *ctree,
			GtkCTreeNode *source_node,
			GtkCTreeNode *new_parent,
			GtkCTreeNode *new_sibling) {

//	gfloat	adj_value;

#define CELL_SPACING 1
#define ROW_TOP(clist, row)        (((clist)->row_height + CELL_SPACING) * (row))

/*
 *	Only leaves shall be dragged.
 */

//	g_message("DragCompareFunc called!\n");

//	g_message("New Parent: %x\n",new_parent);

//	adj_value=GTK_CLIST(table_tree)->vadjustment->value;
//	printf("adj_value: %.2f\n",adj_value);
//	printf("drag_pos: %i\n",ROW_TOP((GTK_CLIST(table_tree)),(GTK_CTREE_ROW(new_parent))) );
//	adj_value+=10.0;
//	gtk_adjustment_set_value(GTK_CLIST(table_tree)->vadjustment,adj_value);

	if (new_parent==NULL) {return FALSE;}

	if (GTK_CTREE_ROW(source_node)->is_leaf) {
		// Dupe-Checking. This might become slow with a big tree, but
		// it works reliably.
		if ((find_node_by_text(ctree,new_parent,GTK_CELL_TEXT(GTK_CTREE_ROW(source_node)->row.cell[0])->text,0))!=NULL)
			return FALSE;
		else	return TRUE;
	} else {
		return FALSE;
	}
}



void on_ac_ok_clicked(GtkWidget *widget, gpointer user_data) {
	char	dbquery[100];
	char	*cat_name;
	char	cat_tname[100];
	
	cat_name=gtk_entry_get_text((GtkEntry *)gnome_entry_gtk_entry(add_category_name));

	assert(cat_name!=NULL);

	gnome_dialog_close(add_category_dialog);

	sprintf(cat_tname,"_cat_%s",cat_name);
	escape_name(cat_tname);		//Now we have an escaped "_cat_xxx" name

	set_stock_names(cat_tname,cat_name);
	
	if (mysql!=NULL) {
		sprintf(dbquery,"create table %s (StockTablename Text)",cat_tname);
		mysql_query(mysql,dbquery);
	}
	update_tablelist(mysql);
}

void on_ac_cancel_clicked(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_close(add_category_dialog);
}

void on_addcategorybutton_clicked(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_run(add_category_dialog);
}

void on_deletestockbutton_clicked(GtkWidget *widget, gpointer user_data) {
	char	*node_name;
	char	*node_tname;
	char	*parent_name;
	char	*parent_tname;
	char	dbquery[100];

	if (selected_node!=NULL)	{
		node_name=GTK_CELL_TEXT(GTK_CTREE_ROW(selected_node)->row.cell[0])->text;
		node_tname=get_stock_tname(node_name,NULL);

		if (GTK_CTREE_ROW(selected_node)->is_leaf) {
			//We need to delete just a Stock. Leaves always have parents, btw.
			parent_name=GTK_CELL_TEXT(GTK_CTREE_ROW(GTK_CTREE_ROW(selected_node)->parent)->row.cell[0])->text;
			parent_tname=get_category_tname(parent_name);
			gtk_ctree_remove_node(table_tree,selected_node);

			// FIXME: when deleting from "All Stocks", we need to check the categories

			sprintf(dbquery,"delete from %s where StockTableName=\'%s\'",parent_tname,node_tname);
			mysql_query(mysql,dbquery);
			sprintf(dbquery,"delete from _TableNames where TName=\'%s\'",node_tname);
			mysql_query(mysql,dbquery);
		} else {
			//We delete a Category
			gtk_ctree_remove_node(table_tree,selected_node);
			sprintf(dbquery,"drop table %s",node_tname);
			mysql_query(mysql,dbquery);
			sprintf(dbquery,"delete from _TableNames where TName=\'%s\'",node_tname);
			mysql_query(mysql,dbquery);
		
			sprintf(dbquery,"update _vtsettings set Category=\'[none]\' where Category=\'%s\'",get_category_name(node_tname));
			mysql_query(mysql,dbquery);

		}
		
	}
}

TradeTime	trading_time;
int	tt_selected_day;

// FIXME: Need to do a sanity check on the times entered

void on_tt_offset_spinbutton_changed(GtkWidget *widget, gpointer user_data) {
	trading_time.ttime[tt_selected_day].offset=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

void on_tt_hstart_spinbutton_changed(GtkWidget *widget, gpointer user_data) {
	trading_time.ttime[tt_selected_day].hstart=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

void on_tt_hend_spinbutton_changed(GtkWidget *widget, gpointer user_data) {
	trading_time.ttime[tt_selected_day].hend=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

void on_tt_mstart_spinbutton_changed(GtkWidget *widget, gpointer user_data) {
	trading_time.ttime[tt_selected_day].mstart=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

void on_tt_mend_spinbutton_changed(GtkWidget *widget, gpointer user_data) {
	trading_time.ttime[tt_selected_day].mend=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}

void on_trading_time_dialog_show(GtkWidget *widget, gpointer user_data) {
	char	*saturday="Saturday";
	char	*friday="Friday";
	char	*thursday="Thursday";
	char	*wednesday="Wednesday";
	char	*tuesday="Tuesday";
	char	*monday="Monday";
	char	*sunday="Sunday";

	// Set up the clist
	gtk_clist_clear(tt_days_clist);
	gtk_clist_prepend(tt_days_clist,&saturday);
	gtk_clist_prepend(tt_days_clist,&friday);
	gtk_clist_prepend(tt_days_clist,&thursday);
	gtk_clist_prepend(tt_days_clist,&wednesday);
	gtk_clist_prepend(tt_days_clist,&tuesday);
	gtk_clist_prepend(tt_days_clist,&monday);
	gtk_clist_prepend(tt_days_clist,&sunday);
	gtk_clist_select_row(tt_days_clist,0,0);
	
}

/*gint on_trading_time_dialog_close(GtkWidget *widget, gpointer user_data) {

	return FALSE;
}
*/
void on_tt_days_clist_select_row(GtkWidget *widget,gint row,gint column,GdkEventButton *event,gpointer data) {

	// Update widgets
	tt_selected_day=row;
	gtk_spin_button_set_value(tt_offset_spinbutton,trading_time.ttime[row].offset);
	gtk_spin_button_set_value(tt_hstart_spinbutton,trading_time.ttime[row].hstart);
	gtk_spin_button_set_value(tt_hend_spinbutton,trading_time.ttime[row].hend);
	gtk_spin_button_set_value(tt_mstart_spinbutton,trading_time.ttime[row].mstart);
	gtk_spin_button_set_value(tt_mend_spinbutton,trading_time.ttime[row].mend);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tt_trading_checkbutton),trading_time.ttime[row].trading);
}

void get_trading_times(TradeTime *t_time,const char *name) {
	char	*tname;
	char	ttname[200];
	char	dbquery[1000];
	int	i;
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;

	tname=get_category_tname(name);
	sprintf(ttname,"_times%s",tname);	// Now ttname is "_times_cat_%s"
	sprintf(dbquery,"select day,trading,offset,hstart,hend,mstart,mend from %s order by day",ttname);
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);

	// Set defaults - Sunday
	t_time->ttime[0].trading=0;
	t_time->ttime[0].offset=-900;
	t_time->ttime[0].hstart=9;
	t_time->ttime[0].mstart=0;
	t_time->ttime[0].hend=20;
	t_time->ttime[0].mend=0;
	// Set defaults - Monday
	t_time->ttime[1].trading=1;
	t_time->ttime[1].offset=-900;
	t_time->ttime[1].hstart=9;
	t_time->ttime[1].mstart=0;
	t_time->ttime[1].hend=20;
	t_time->ttime[1].mend=0;
	// Set defaults - Tuesday
	t_time->ttime[2].trading=1;
	t_time->ttime[2].offset=-900;
	t_time->ttime[2].hstart=9;
	t_time->ttime[2].mstart=0;
	t_time->ttime[2].hend=20;
	t_time->ttime[2].mend=0;
	// Set defaults - Wednesday
	t_time->ttime[3].trading=1;
	t_time->ttime[3].offset=-900;
	t_time->ttime[3].hstart=9;
	t_time->ttime[3].mstart=0;
	t_time->ttime[3].hend=20;
	t_time->ttime[3].mend=0;
	// Set defaults - Thursday
	t_time->ttime[4].trading=1;
	t_time->ttime[4].offset=-900;
	t_time->ttime[4].hstart=9;
	t_time->ttime[4].mstart=0;
	t_time->ttime[4].hend=20;
	t_time->ttime[4].mend=0;
	// Set defaults - Friday
	t_time->ttime[5].trading=1;
	t_time->ttime[5].offset=-900;
	t_time->ttime[5].hstart=9;
	t_time->ttime[5].mstart=0;
	t_time->ttime[5].hend=20;
	t_time->ttime[5].mend=0;
	// Set defaults - Saturday
	t_time->ttime[6].trading=0;
	t_time->ttime[6].offset=-900;
	t_time->ttime[6].hstart=9;
	t_time->ttime[6].mstart=0;
	t_time->ttime[6].hend=20;
	t_time->ttime[6].mend=0;

	if (result_set!=NULL) {
		for (i=0;i<7;i++) {
			if ((result_row=mysql_fetch_row(result_set))!=NULL) {
				t_time->ttime[atoi(result_row[0])].trading=atoi(result_row[1]);
				t_time->ttime[atoi(result_row[0])].offset=atoi(result_row[2]);
				t_time->ttime[atoi(result_row[0])].hstart=atoi(result_row[3]);
				t_time->ttime[atoi(result_row[0])].hend=atoi(result_row[4]);
				t_time->ttime[atoi(result_row[0])].mstart=atoi(result_row[5]);
				t_time->ttime[atoi(result_row[0])].mend=atoi(result_row[6]);
			}
		}
	}
}

void on_tradingtimebutton_clicked(GtkWidget *widget, gpointer user_data) {
	char	*name;
	char	*tname;
	char	ttname[200];
	char	dbquery[1000];
	int	i;

	// Get Category Table name
//	gtk_ctree_node_get_text(GTK_CTREE(table_tree),GTK_CTREE_NODE(selected_node),0,&name);
	name=GTK_CELL_TEXT(GTK_CTREE_ROW(selected_node)->row.cell[0])->text;

	get_trading_times(&trading_time,name);

	tname=get_category_tname(name);
	sprintf(ttname,"_times%s",tname);	// Now ttname is "_times_cat_%s"
	if (gnome_dialog_run_and_close(trading_time_dialog)==0) {
		// Ok clicked - save the modified data to the database
		sprintf(dbquery,"drop table %s",ttname);
		mysql_query(mysql,dbquery);

		sprintf(dbquery,"create table %s (day int,offset int,hstart int,hend int,mstart int,mend int,trading int)",ttname);
		mysql_query(mysql,dbquery);

		for (i=0;i<7;i++) {
			sprintf(dbquery,"insert into %s (day,offset,hstart,hend,mstart,mend,trading) values (%i,%i,%i,%i,%i,%i,%i)",
					ttname,i,
					trading_time.ttime[i].offset,
					trading_time.ttime[i].hstart,
					trading_time.ttime[i].hend,
					trading_time.ttime[i].mstart,
					trading_time.ttime[i].mend,
					trading_time.ttime[i].trading);
					       
			mysql_query(mysql,dbquery);
		}
	}
}

void on_tt_trading_checkbutton_toggled(GtkWidget *widget, gpointer data) {
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		gtk_widget_set_sensitive(GTK_WIDGET(tt_hstart_spinbutton),1);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_hend_spinbutton),1);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_mstart_spinbutton),1);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_mend_spinbutton),1);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_offset_spinbutton),1);
		trading_time.ttime[tt_selected_day].trading=1;
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(tt_hstart_spinbutton),0);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_hend_spinbutton),0);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_mstart_spinbutton),0);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_mend_spinbutton),0);
		gtk_widget_set_sensitive(GTK_WIDGET(tt_offset_spinbutton),0);
		trading_time.ttime[tt_selected_day].trading=0;
	}
}

/*
 *	Callback for selecting a row in the main CTree widget
 *	which contains the table names from the database.
 *	In/Out:	Std. Gtk+ callback for CList widget.
 */

void on_tabletree_select_row(GtkCTree *ctree,
                               GtkCTreeNode *row,
                               gint column) {


	if (GTK_CTREE_ROW(row)->is_leaf) {
		gtk_widget_set_sensitive(GTK_WIDGET(view_chart_button),1);
	}
	
	//Detail
	if ((GTK_CTREE_ROW(row)->is_leaf) || (row==all_stocks_ctree_node)) {
		gtk_widget_set_sensitive(GTK_WIDGET(trading_time_button),0);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(trading_time_button),1);
	}

	if ((row==all_stocks_ctree_node) || (GTK_CTREE_ROW(row)->parent==all_stocks_ctree_node)) {
		gtk_widget_set_sensitive(GTK_WIDGET(delete_stock_button),0);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(delete_stock_button),1);
	}

	selected_node=row;

}

/*
 *	Callback for unselecting a row in the main CTree widget
 *	containing the table names from the db.
 *	In/Out:	Std. Gtk+ callback for CList widget.
 */

void on_tabletree_unselect_row(GtkCTree *ctree,
                               GtkCTreeNode *row,
                               gint column) {

	gtk_widget_set_sensitive(GTK_WIDGET(delete_stock_button),0);
	gtk_widget_set_sensitive(GTK_WIDGET(trading_time_button),0);
	gtk_widget_set_sensitive(GTK_WIDGET(view_chart_button),0);

	selected_node=NULL;

//	g_message("Unselected sthg.\n");
}

// This is used for Categories AND for Stocks

void set_stock_names(const char *tname,const char *name) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	char	tname_e[300];
	char	name_e[300];
	gpointer origkey,value;

	assert(tname!=NULL);
	assert(name!=NULL);

	// Cache consistency
	if (g_hash_table_lookup_extended(name_cache,tname,&origkey,&value)==TRUE) {
		g_hash_table_remove(name_cache,tname);
		g_free(origkey);
		g_free(value);
	}
	g_hash_table_insert(name_cache,strdup(tname),strdup(name));

	if (g_hash_table_lookup_extended(tname_cache,name,&origkey,&value)==TRUE) {
		g_hash_table_remove(tname_cache,name);
		g_free(origkey);
		g_free(value);
	}
	g_hash_table_insert(tname_cache,strdup(name),strdup(tname));
	// ---------------
	
	mysql_escape_string(tname_e,tname,strlen(tname));
	mysql_escape_string(name_e,name,strlen(name));

	sprintf(dbquery,"select Name from _TableNames where TName=\'%s\'",tname_e);
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);
	if (mysql_num_rows(result_set)==0) {
		sprintf(dbquery,"insert into _TableNames (TName,Name) VALUES(\'%s\',\'%s\')",tname_e,name_e);		
		mysql_query(mysql,dbquery);
	} else {
		// If name already exists, update the entry
		sprintf(dbquery,"update _TableNames set Name=\'%s\'쟷here TName=\'%s\'",name_e,tname_e);
		mysql_query(mysql,dbquery);
	}
	mysql_free_result(result_set);
}

//int	cachemiss=0;

char *get_stock_name(const char *tname) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	char	tname_e[300];
	char	*name;

	assert(tname!=NULL);

	name=g_hash_table_lookup(name_cache,tname);

	if (name==NULL) {
//		cachemiss++;
		mysql_escape_string(tname_e,tname,strlen(tname));
		sprintf(dbquery,"select Name from _TableNames where TName=\'%s\'",tname_e);		
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);
		if (mysql_num_rows(result_set)==0) {
			mysql_free_result(result_set);	
			return strdup(tname);
		}
		result_row=mysql_fetch_row(result_set);
		name=strdup(result_row[0]);
		mysql_free_result(result_set);	
		if (g_hash_table_lookup(tname_cache,name)==NULL) {
			g_hash_table_insert(tname_cache,strdup(name),strdup(tname));
//			printf("tname_cache/");
		}
		g_hash_table_insert(name_cache,strdup(tname),strdup(name));
//		printf("name_cache: added %s/%s\n",name,tname);
		return name;
	} else return strdup(name);
}

/*
 *	error can be NULL
 */

char *get_stock_tname(const char *name,int *error) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	char	name_e[300];
	char	*tname;
	char	*result;

	assert(name!=NULL);

	tname=g_hash_table_lookup(tname_cache,name);
	if (tname==NULL) {
//		printf("tname_cache miss: %s\n",name);
		
		mysql_escape_string(name_e,name,strlen(name));

		sprintf(dbquery,"select TName from _TableNames where Name=\'%s\'",name_e);		
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);
		if (mysql_num_rows(result_set)==0) {
			if (error!=NULL) *error=1;
			result=escape_name(strdup(name));
		} else {
			result_row=mysql_fetch_row(result_set);
			result=strdup(result_row[0]);
		}
		mysql_free_result(result_set);	
		return result;
	} else return strdup(tname);
}

char *get_category_name(const char *tname) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	char	*name;

	assert(tname!=NULL);

	sprintf(dbquery,"select Name from _TableNames where TName=\'%s\' AND TName like	\'\\_cat\\_%%\'",tname);		
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);
	if (mysql_num_rows(result_set)==0) {
		mysql_free_result(result_set);	
		return strdup(tname);
	}
	result_row=mysql_fetch_row(result_set);
	name=strdup(result_row[0]);
	mysql_free_result(result_set);	
	return name;
}

char *get_category_tname(const char *name) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	char	*tname;

	assert(name!=NULL);

	sprintf(dbquery,"select TName from _TableNames where Name=\'%s\' AND TName like \'\\_cat\\_%%\'",name);		
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);
	if (mysql_num_rows(result_set)==0) {
		mysql_free_result(result_set);	
		return strdup(name);
	}
	result_row=mysql_fetch_row(result_set);
	tname=strdup(result_row[0]);
	mysql_free_result(result_set);
	return tname;
}

/*
 *	This function rebuilds the main CList widget. It queries
 *	the database to get all the table names and displays those
 *	which do not begin with an underscore.
 *	In:	*mysql:	The mysql object of the current db connection.
 */

void update_tablelist(MYSQL *mysql) {
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	MYSQL_RES *result_set2;
	MYSQL_ROW result_row2;
	char	dbquery[100];
	char	dbquery2[100];
	char	list_items[4][50];
	char	*list_data[4];
	int	i;
	GtkCTreeNode *temp_parent;
	char	*temp_string;

	for (i=0;i<3;i++)
	{
		list_data[i]=list_items[i];
	}

	if (mysql!=NULL) {
	
		gtk_clist_freeze(GTK_CLIST(table_tree));
		
		sprintf(dbquery,"show tables");		
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);

		gtk_clist_clear(GTK_CLIST(table_tree));

	// create the "All Stocks" branch which has all stocks.
		
	// FIXME: Show comments here too... (how ???)		
		
		if (mysql_errno(mysql)==0) {

			sprintf(list_items[0],"All Stocks");
			sprintf(list_items[1]," ");
			sprintf(list_items[2]," ");
			list_data[3]=NULL;

			all_stocks_ctree_node=gtk_ctree_insert_node(table_tree,
				NULL,		//parent
				NULL,		//sibling
				&list_data[0],	//text
				5,		//spacing
				NULL,
				NULL,
				NULL,
				NULL,
				FALSE,		//isleaf
				FALSE);		//expanded


			while ((result_row=mysql_fetch_row(result_set))!=NULL) {
				if (result_row[0][0]!='_') {
					temp_string=get_stock_name(result_row[0]);

					sprintf(list_items[0],"%s",temp_string);
					sprintf(list_items[1],"-");
					sprintf(list_items[2],"-");
					list_data[3]=NULL;
					g_free(temp_string);


					gtk_ctree_insert_node(table_tree,
						all_stocks_ctree_node,	//parent
						NULL,		//sibling
						&list_data[0],	//text
						5,		//spacing
						NULL,
						NULL,
						NULL,
						NULL,
						TRUE,		//isleaf
						FALSE);		//expanded
				}
			}
		}

		gtk_ctree_sort_node(table_tree,all_stocks_ctree_node);
		
	// create the hierarchical Structure

	// FIXME: Show comments

		mysql_free_result(result_set);

		sprintf(dbquery,"show tables like \'%s\'","\\_cat\\_%%"); //No other way
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);
			while ((result_row=mysql_fetch_row(result_set))!=NULL) {
				temp_string=get_category_name(result_row[0]);

				sprintf(list_items[0],"%s",temp_string);
				sprintf(list_items[1]," ");
				sprintf(list_items[2],"-");
				list_data[3]=NULL;
				free(temp_string);


				temp_parent= gtk_ctree_insert_node(table_tree,
					NULL,		//parent
					NULL,		//sibling
					&list_data[0],	//text
					5,		//spacing
					NULL,
					NULL,
					NULL,
					NULL,
					FALSE,		//isleaf
					FALSE);		//expanded
				
				sprintf(dbquery2,"select StockTableName from %s",result_row[0]);
				mysql_query(mysql,dbquery2);
				result_set2=mysql_store_result(mysql);
				while ((result_row2=mysql_fetch_row(result_set2))!=NULL) {
					temp_string=get_stock_name(result_row2[0]);

					sprintf(list_items[0],"%s",temp_string);
					sprintf(list_items[1],"-");
					sprintf(list_items[2],"-");
					list_data[3]=NULL;
					free(temp_string);
					gtk_ctree_insert_node(table_tree,
						temp_parent,	//parent
						NULL,		//sibling
						&list_data[0],	//text
						5,		//spacing
						NULL,
						NULL,
						NULL,
						NULL,
						TRUE,		//isleaf
						FALSE);		//expanded
					
				}
				gtk_ctree_sort_node(table_tree,temp_parent);
				mysql_free_result(result_set2);
			}

		mysql_free_result(result_set);
		
		gtk_clist_thaw(GTK_CLIST(table_tree));
	} else {
		gtk_clist_clear(GTK_CLIST(table_tree));
		gtk_widget_set_sensitive(GTK_WIDGET(delete_stock_button),0);
	}
}

/*
 *	General-purpose error function. It displays a GnomeDialog
 *	with an error-message and an Ok-button.
 *	In:	*message: Pointer to the message to be displayed.
 */

void error_dialog(const char *message) {
	GnomeDialog *dialog;

	assert(message!=NULL);

	dialog=(GnomeDialog *)gnome_message_box_new(message,
				GNOME_MESSAGE_BOX_QUESTION,
				GNOME_STOCK_BUTTON_OK,
				NULL);
		
	gnome_dialog_run_and_close (GNOME_DIALOG (dialog));	
	
}

/*
 *	General-purpose function. It displays a GnomeDialog
 *	with a message and 2 buttons: ok & cancel.
 *	In:	*message: Pointer to the message to be displayed.
 *	Out:	int:	  0 = user clicked Ok.
 *			  1 = user clicked cancel.
 *			  anything else = user closed window
 */

int ok_cancel_dialog(const char *message) {
	GnomeDialog *dialog;
	int	ret;

	assert(message!=NULL);

	dialog=(GnomeDialog *)gnome_message_box_new(message,
				GNOME_MESSAGE_BOX_QUESTION,
				GNOME_STOCK_BUTTON_OK,
				GNOME_STOCK_BUTTON_CANCEL,
				NULL);
		
	ret=gnome_dialog_run_and_close (GNOME_DIALOG (dialog));	
	return ret;
}

/*
 *	This is a callback function that is used when the user closes
 *	the main program window.
 *	In/Out:	Std. Gtk+ callback function.
 */

void on_charter_delete_event(GtkWidget *widget, gpointer user_data) {
	if (mysql!=NULL) {
//		g_message("Disconnecting from mysql Server\n");
		mysql_close(mysql);
		mysql=NULL;
	}
	gtk_main_quit();
}

/*
 *	The main function sets up many of the widgets. It gets a pointer
 *	to a widget from libglade, which gets it from the XML-File.
 *	In/Out:	Std. C
 */

int main(int argc, char *argv[]) {
	int	i;
	char	*gchartman_version=VERSION;
	GtkWidget *chart_xscale_plus;
	GtkWidget *chart_xscale_minus;
	GtkWidget *chart_yscale_plus;
	GtkWidget *chart_yscale_minus;
	GtkWidget *icon;
	char	*homedir;
	char	homechartdir[250];

//	instance_list=NULL;
	quote_vector=NULL;

	gtk_init(&argc,&argv);
	gnome_init("gChartman",gchartman_version,argc,argv);
	glade_gnome_init();

	if (getenv("HOME")==NULL) {
		g_print("Your $HOME environment variable is not set!\n");
		exit(6);
	}

	homedir=strdup(getenv("HOME"));
	sprintf(homechartdir,"%s/.gchartman",homedir);

	mkdir(homechartdir,S_IRWXU);

	name_cache=g_hash_table_new(g_str_hash,g_str_equal);
	tname_cache=g_hash_table_new(g_str_hash,g_str_equal);

	/* load the interface */
	xml=glade_xml_new(GLADE_FILE, NULL);

      	/* connect the signals in the interface */
      	glade_xml_signal_autoconnect(xml);

	about_box=(GnomeDialog *)glade_xml_get_widget(xml, "Charter_about");
	vt_box=glade_xml_get_widget(xml,"Charter_VTWin");
	desired_vtpage=glade_xml_get_widget(xml,"desired_vt_page");
	vt_select_area=(GnomeCanvas *)glade_xml_get_widget(xml,"VTSelectArea");
	page_list=(GtkCList *)glade_xml_get_widget(xml,"PageList");
	table_tree=(GtkCTree *)glade_xml_get_widget(xml,"TableTree");
	gtk_clist_set_reorderable(GTK_CLIST(table_tree),TRUE);

	chart_structure_ctree=(GtkCTree *)glade_xml_get_widget(xml,"chart_structure_ctree");
	
	add_page_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"AddPageDialog");
	add_page_notebook=(GtkNotebook *)glade_xml_get_widget(xml,"AddPageNoteBook");
	single_page_num_spin=(GtkSpinButton *)glade_xml_get_widget(xml,"SinglePageNumber");
	multi_page_num_spin=(GtkSpinButton *)glade_xml_get_widget(xml,"MultiPageNumber");
	multi_page_start_spin=(GtkSpinButton *)glade_xml_get_widget(xml,"MultiPageStart");
	multi_page_end_spin=(GtkSpinButton *)glade_xml_get_widget(xml,"MultiPageEnd");
	vtpage_displayed_label=(GtkLabel *)glade_xml_get_widget(xml,"VTPageDisplay");
	connectdb_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"ConnectDBDialog");
	connectdb_host=(GnomeEntry *)glade_xml_get_widget(xml,"ConnectDBHost");
	connectdb_user=(GnomeEntry *)glade_xml_get_widget(xml,"ConnectDBUser");
	connectdb_password=(GtkEntry *)glade_xml_get_widget(xml,"ConnectDBPassword");
	connectdb_database=(GnomeEntry *)glade_xml_get_widget(xml,"ConnectDBDatabase");
	add_category_name=(GnomeEntry *)glade_xml_get_widget(xml,"AC_Categoryname");
	database_status_label=(GtkLabel *)glade_xml_get_widget(xml,"DataBaseStatusLabel");
	disable_button=(GtkToggleButton *)glade_xml_get_widget(xml,"DisableButton");
	vt_up=(GtkButton *)glade_xml_get_widget(xml,"VT_UP");
	vt_down=(GtkButton *)glade_xml_get_widget(xml,"VT_DOWN");
	vt_left=(GtkButton *)glade_xml_get_widget(xml,"VT_LEFT");
	vt_right=(GtkButton *)glade_xml_get_widget(xml,"VT_RIGHT");
	vt_taller=(GtkButton *)glade_xml_get_widget(xml,"VT_TALLER");
	vt_smaller=(GtkButton *)glade_xml_get_widget(xml,"VT_SMALLER");

	chart_window=(GtkWidget *)glade_xml_get_widget(xml,"ChartWindow");
	chart_canvas=(GnomeCanvas *)glade_xml_get_widget(xml,"ChartCanvas");
	gtk_signal_connect (GTK_OBJECT (gnome_canvas_root(chart_canvas)), "event",GTK_SIGNAL_FUNC(on_chartcanvas_root_event),NULL);

	chart_xscale_plus=(GtkWidget *)glade_xml_get_widget(xml,"Chart_XScalePlus");
	chart_xscale_minus=(GtkWidget *)glade_xml_get_widget(xml,"Chart_XScaleMinus");
	chart_yscale_plus=(GtkWidget *)glade_xml_get_widget(xml,"Chart_YScalePlus");
	chart_yscale_minus=(GtkWidget *)glade_xml_get_widget(xml,"Chart_YScaleMinus");
// add icons
	icon=gnome_pixmap_new_from_file(PLUGINDIR"/zoomxplus.png");
	gtk_container_remove(GTK_CONTAINER(chart_xscale_plus),GTK_BIN(chart_xscale_plus)->child);
	gtk_container_add(GTK_CONTAINER(chart_xscale_plus),icon);
	gtk_widget_show(icon);

	icon=gnome_pixmap_new_from_file(PLUGINDIR"/zoomxminus.png");
	gtk_container_remove(GTK_CONTAINER(chart_xscale_minus),GTK_BIN(chart_xscale_minus)->child);
	gtk_container_add(GTK_CONTAINER(chart_xscale_minus),icon);
	gtk_widget_show(icon);

	icon=gnome_pixmap_new_from_file(PLUGINDIR"/zoomyplus.png");
	gtk_container_remove(GTK_CONTAINER(chart_yscale_plus),GTK_BIN(chart_yscale_plus)->child);
	gtk_container_add(GTK_CONTAINER(chart_yscale_plus),icon);
	gtk_widget_show(icon);

	icon=gnome_pixmap_new_from_file(PLUGINDIR"/zoomyminus.png");
	gtk_container_remove(GTK_CONTAINER(chart_yscale_minus),GTK_BIN(chart_yscale_minus)->child);
	gtk_container_add(GTK_CONTAINER(chart_yscale_minus),icon);
	gtk_widget_show(icon);
//----------
	chart_scrolled_window=(GtkScrolledWindow *)glade_xml_get_widget(xml,"ChartScrolledWindow");
	
	add_category_button=(GtkButton *)glade_xml_get_widget(xml,"AddCategoryButton");
	delete_stock_button=(GtkButton *)glade_xml_get_widget(xml,"DeleteStockButton");
	disconnect_button=(GtkButton *)glade_xml_get_widget(xml,"disconnect_button");
	connect_button=(GtkButton *)glade_xml_get_widget(xml,"connect_button");
	view_chart_button=(GtkButton *)glade_xml_get_widget(xml,"ChartButton");

	add_category_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"AddCategoryDialog");

	vt_timeout_window=(GtkWidget *)glade_xml_get_widget(xml,"VTTimeOutWindow");
	vt_progress_bar=(GtkProgress *)glade_xml_get_widget(xml,"VTProgressBar");

	chart_type_toolbar=(GtkToolbar *)glade_xml_get_widget(xml,"ChartTypeToolbar");
	connectdb_ok=(GtkWidget *)glade_xml_get_widget(xml,"ConnectDB_OK");

	get_config_name_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"GetConfigNameDialog");
	new_config_name_entry=(GnomeEntry *)glade_xml_get_widget(xml,"NewConfigNameEntry");
	new_config_pos_button=(GtkWidget *)glade_xml_get_widget(xml,"NewConfigPosButton");

	unknown_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"unknown_dialog");
	unknown_stock_name=(GnomeEntry *)glade_xml_get_widget(xml,"unknown_stock_name");
	unknown_stock_name_entry=(GtkEntry *)glade_xml_get_widget(xml,"unknown_stock_name_entry");
	unknown_stock_tname=(GtkEntry *)glade_xml_get_widget(xml,"unknown_stock_tname");
	unknown_stock_value=(GnomeEntry *)glade_xml_get_widget(xml,"unknown_stock_value");
	unknown_stock_maximum=(GnomeEntry *)glade_xml_get_widget(xml,"unknown_stock_maximum");
	unknown_stock_minimum=(GnomeEntry *)glade_xml_get_widget(xml,"unknown_stock_minimum");
	unknown_stock_volume=(GnomeEntry *)glade_xml_get_widget(xml,"unknown_stock_volume");
	tname_ok_checkbutton=(GtkCheckButton *)glade_xml_get_widget(xml,"tname_ok_checkbutton");
	gtk_widget_set_sensitive(GTK_WIDGET(tname_ok_checkbutton),FALSE);
	
	vtwin_appbar=(GnomeAppBar *)glade_xml_get_widget(xml,"VTWin_AppBar");
	
	unknown_dialog_existing_stocks_clist=(GtkCList *)glade_xml_get_widget(xml,"unknown_dialog_existing_stocks_clist");

	global_config_name_combo=(GtkCombo *)glade_xml_get_widget(xml,"global_config_name_combo");
	local_config_name_combo=(GtkCombo *)glade_xml_get_widget(xml,"local_config_name_combo");

	chart_cat_combo=(GtkCombo *)glade_xml_get_widget(xml,"Chart_cat_combo");
	chart_stock_combo=(GtkCombo *)glade_xml_get_widget(xml,"Chart_stock_combo");
	
	trading_time_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"trading_time_dialog");
	tt_offset_spinbutton=(GtkSpinButton *)glade_xml_get_widget(xml,"TT_offset_spinbutton");
	tt_hstart_spinbutton=(GtkSpinButton *)glade_xml_get_widget(xml,"TT_hstart_spinbutton");
	tt_hend_spinbutton=(GtkSpinButton *)glade_xml_get_widget(xml,"TT_hend_spinbutton");
	tt_mstart_spinbutton=(GtkSpinButton *)glade_xml_get_widget(xml,"TT_mstart_spinbutton");
	tt_mend_spinbutton=(GtkSpinButton *)glade_xml_get_widget(xml,"TT_mend_spinbutton");
	trading_time_button=(GtkButton *)glade_xml_get_widget(xml,"TradingTimeButton");
	tt_days_clist=(GtkCList *)glade_xml_get_widget(xml,"TT_days_clist");
	tt_trading_checkbutton=(GtkCheckButton *)glade_xml_get_widget(xml,"TT_trading_checkbutton");

	instance_delete_button=(GtkButton *)glade_xml_get_widget(xml,"instance_delete_button");
	instance_properties_button=(GtkButton *)glade_xml_get_widget(xml,"instance_properties_button");

	edit_comment_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"edit_comment_dialog");
	comment_textbox=(GtkText *)glade_xml_get_widget(xml,"comment_textbox");

	split_stock_dialog=(GnomeDialog *)glade_xml_get_widget(xml,"split_stock_dialog");
	split_stock_upper_spin=(GtkSpinButton *)glade_xml_get_widget(xml,"split_stock_upper_spin");
	split_stock_lower_spin=(GtkSpinButton *)glade_xml_get_widget(xml,"split_stock_lower_spin");
	split_stock_date_edit=(GnomeDateEdit *)glade_xml_get_widget(xml,"split_stock_date_edit");

	chart_appbar=(GnomeAppBar *)glade_xml_get_widget(xml,"chart_appbar");
	
	//Important:
	
	gnome_dialog_close_hides(about_box,TRUE);
	gnome_dialog_close_hides(add_page_dialog,TRUE);
	gnome_dialog_close_hides(connectdb_dialog,TRUE);
	gnome_dialog_close_hides(add_category_dialog,TRUE);
	gnome_dialog_close_hides(unknown_dialog,TRUE);
	gnome_dialog_close_hides(edit_comment_dialog,TRUE);
	gnome_dialog_close_hides(split_stock_dialog,TRUE);

	select_category_combo=(GtkCombo *)glade_xml_get_widget(xml,"VTWin_SelectCategory_ComboBox");

	gtk_ctree_set_drag_compare_func(table_tree,compare_drag_func);

	//Create the Text Item in the Canvas
	vtsa_text=gnome_canvas_item_new (gnome_canvas_root(vt_select_area),
                               GNOME_TYPE_CANVAS_TEXT,
                               "text", "No VT Page loaded yet.",
                               "x", 0.0,
                               "y", 0.0,
//			       "x_offset",0.0,
//			       "y_offset",0.0,
                               "font", "-b&h-lucidatypewriter-medium-r-normal-*-*-120-*-*-m-*-iso8859-1",
                               "anchor", GTK_ANCHOR_NW,
                               "fill_color", "black",
                               NULL);

	gnome_canvas_item_raise_to_top(vtsa_text);

	selected_row=-1;

	xsize=6;	//TODO: Get this Data from Font (or canvas_text maybe ?)
	ysize=12;
	xstep=7;
	ystep=13;

	for (i=0;i<NUM_SELS;i++) {
		points1[i]=gnome_canvas_points_new(2);
		points1[i]->coords[0]=0.0;
		points1[i]->coords[1]=0.0;
		points1[i]->coords[2]=0.0;
		points1[i]->coords[3]=0.0;

		lines1[i]=gnome_canvas_item_new(gnome_canvas_root(vt_select_area),
			GNOME_TYPE_CANVAS_LINE,
			"points",points1[i],
			NULL);	

		gnome_canvas_item_raise_to_top(lines1[i]);
//		gnome_canvas_item_raise(Lines1[i],1);

		points2[i]=gnome_canvas_points_new(2);
		points2[i]->coords[0]=0.0;
		points2[i]->coords[1]=0.0;
		points2[i]->coords[2]=0.0;
		points2[i]->coords[3]=0.0;

		lines2[i]=gnome_canvas_item_new(gnome_canvas_root(vt_select_area),
			GNOME_TYPE_CANVAS_LINE,
			"points",points2[i],
			NULL);	

		gnome_canvas_item_raise_to_top(lines2[i]);
//		gnome_canvas_item_raise(Lines1[i],1);

		rect[i]=gnome_canvas_item_new(gnome_canvas_root(vt_select_area),
					GNOME_TYPE_CANVAS_RECT,
					"x1", 0.0,
					"y1", 0.0,
					"x2", 0.0,
					"y2", 0.0,
					"outline_color",NULL,
					"fill_color","blue",
					NULL);

		gnome_canvas_item_lower_to_bottom(rect[i]);

	}

	gnome_canvas_item_set(rect[SEL_NAME],"fill_color","SkyBlue1",NULL);
	gnome_canvas_item_set(rect[SEL_VALUE],"fill_color","SeaGreen1",NULL);
	gnome_canvas_item_set(rect[SEL_VOLUME],"fill_color","khaki1",NULL);
	gnome_canvas_item_set(rect[SEL_MINIMUM],"fill_color","RosyBrown1",NULL);
	gnome_canvas_item_set(rect[SEL_MAXIMUM],"fill_color","plum3",NULL);


//	UpdateSelection();
		

//	gtk_clist_append(PageList,PageListDefault);
	vtpage_header.previous=NULL;
	vtpage_header.next=NULL;

	init_modules();

	xmlInitParser();
	xmlIndentTreeOutput=TRUE;
	xmlSubstituteEntitiesDefault(TRUE);
	xmlKeepBlanksDefault(TRUE);
	xmlInitializePredefinedEntities();

	sprintf(global_configs_filename,"%s/globalconfigs.xml",homechartdir);
	global_configs_doc=xmlParseFile(global_configs_filename);
	if (global_configs_doc==NULL) {
		global_configs_doc=xmlNewDoc("1.0");
		global_configs_doc->children=xmlNewNode(NULL,"configurations");
	}

	sprintf(local_configs_filename,"%s/localconfigs.xml",homechartdir);
	local_configs_doc=xmlParseFile(local_configs_filename);
	if (local_configs_doc==NULL) {
		local_configs_doc=xmlNewDoc("1.0");
		local_configs_doc->children=xmlNewNode(NULL,"configurations");
	}
	
      	/* start the event loop */
      	gtk_main();

	xmlIndentTreeOutput=TRUE;
	xmlSaveFile(global_configs_filename,global_configs_doc);
	xmlSaveFile(local_configs_filename,local_configs_doc);

	// FIXME: Check what else needs freeing
	g_list_free(type_list);
	if (name_cache!=NULL) g_hash_table_destroy(name_cache);
	if (tname_cache!=NULL) g_hash_table_destroy(tname_cache);
	
      	return 0;
}
