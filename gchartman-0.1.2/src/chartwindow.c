#include <glade/glade.h>
#include <dlfcn.h>
#include <gnome.h>
#include <math.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/entities.h>
#include "chartwindow.h"
#include "gchartman.h"
#include "module.h"

double	c_x,c_y,c_x2,c_y2;
int	_index;

double	quote_maximum;
char	*_selected_table_name;

xmlNodePtr stock_local_configs_node;

void interpolate_voids(Quote *qv,int quote_count) {
	int	i,j;
	int	hole_start=0;
	int	hole_end=0;
	int	hole_len=0;
	double	start_value;
	double	end_value;
	double	delta;
	
	if (quote_count>2) {
		i=1;
		while (i<quote_count) {
			// Find last valid before hole
			while (qv[i].valid && i<quote_count) i++;
			hole_start=i-1;
			
			if (i<quote_count) {
				// Find first valid after hole
				while (!qv[i].valid) i++;
				hole_end=i;
				
				hole_len=hole_end-hole_start-1;
				start_value=qv[hole_start].value;
				end_value=qv[hole_end].value;
				delta=(end_value-start_value)/(hole_len+1);
			
				for (j=hole_start+1;j<hole_end;j++) {
					qv[j].value=qv[j-1].value+delta;
					qv[j].valid=TRUE;
					qv[j].interpolated=TRUE;
				}		
			}
		}
	}
	
}

void init_stock_combo(char *cat_name,char *select) {
	GList	*stocks_list;
	char	dbquery[1000];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;

	stocks_list=NULL;
	if (strcmp(cat_name,"All Stocks")==0) {
		sprintf(dbquery,"show tables");
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);

		while ((result_row=mysql_fetch_row(result_set))!=NULL) {
			if (*result_row[0]!='_')
				stocks_list=g_list_append(stocks_list,get_stock_name(result_row[0]));
		}

	} else {
		sprintf(dbquery,"select StockTableName from %s order by StockTableName",get_category_tname(cat_name));
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);

		while ((result_row=mysql_fetch_row(result_set))!=NULL) {
			stocks_list=g_list_append(stocks_list,get_stock_name(result_row[0]));
		}
	}

	gtk_combo_set_popdown_strings(chart_stock_combo,stocks_list);
	if (select!=NULL) {
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(chart_stock_combo)->entry), select);
	}
	g_list_free(stocks_list);
	mysql_free_result(result_set);	
}

void free_ctree_item(GtkCTree *ctree, GtkCTreeNode *node, gpointer data) {
	ChartInstance	*ci;
	
	ci=(ChartInstance *)gtk_ctree_node_get_row_data(ctree,node);
	ci->type->free_object(ci);
	g_free(ci->quote_vector);
	g_free(ci);
	gtk_ctree_remove_node(ctree,node);
}

void free_chart_instances(void) {

	gtk_ctree_post_recursive(chart_structure_ctree,NULL,free_ctree_item,NULL);
}

void create_defaults (gpointer node_data, gpointer user_data) {
	ChartType *ct=(ChartType *)node_data;
	ChartInstance *ci;

	if (ct->caps->default_create) {
		ci=add_chart_instance(ct,NULL,NULL,quote_vector,quote_count);
		ci->parent=NULL;
	}
}

int create_quote_vector(char *stock_name,char *cat_name,Quote **quote_vector) {
	char	dbquery[1000];
	char	*stock_tname;
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	MYSQL_RES *result_set2;
	MYSQL_ROW result_row2;
	struct tm *tm_time;
	time_t	max_timestamp,min_timestamp;
	TradeTime	t_time;
	int	i;
	int	quote_count;

	get_trading_times(&t_time,cat_name);

	stock_tname=get_stock_tname(stock_name,NULL);

	if (*quote_vector!=NULL) {
		g_free(*quote_vector);
		*quote_vector=NULL;
	}
	quote_count=0;

	quote_maximum=0.0;

	_selected_table_name=strdup(stock_tname);

	sprintf(dbquery,"select UNIX_TIMESTAMP(MAX(time)),UNIX_TIMESTAMP(MIN(time)),TO_DAYS(MAX(time))-TO_DAYS(MIN(time)) from %s",stock_tname);
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);
	if ((result_row=mysql_fetch_row(result_set))!=NULL) {

		// Kludge: loop once to find number of days then loop again to fill memory

		quote_count=0;
		max_timestamp=atoi(result_row[0]);
		min_timestamp=atoi(result_row[1]);
		for (;max_timestamp>(min_timestamp-86400);max_timestamp-=86400) {
			tm_time=localtime(&max_timestamp);

			if (t_time.ttime[tm_time->tm_wday].trading) {
				quote_count++;
			}
		}

		*quote_vector=(Quote *)malloc(sizeof(Quote)*(quote_count+1));

		quote_count=0;
		max_timestamp=atoi(result_row[0]);
		min_timestamp=atoi(result_row[1]);
		for (;max_timestamp>(min_timestamp-86400);max_timestamp-=86400) {
			tm_time=localtime(&max_timestamp);

			if (t_time.ttime[tm_time->tm_wday].trading) {
				quote_count++;
				(*quote_vector)[quote_count].day=tm_time->tm_mday;
				(*quote_vector)[quote_count].month=tm_time->tm_mon;
				(*quote_vector)[quote_count].year=tm_time->tm_year;
				(*quote_vector)[quote_count].wday=tm_time->tm_wday;
				(*quote_vector)[quote_count].valid=FALSE;
				(*quote_vector)[quote_count].interpolated=FALSE;
			}
		}
		
		sprintf(dbquery,"select UNIX_TIMESTAMP(time),value,YEAR(time),MONTH(time),DAYOFMONTH(time) from %s order by time desc",stock_tname);
		mysql_query(mysql,dbquery);
		result_set2=mysql_store_result(mysql);
		while ((result_row2=mysql_fetch_row(result_set2))!=NULL) {
			for (i=1;i<quote_count+1;i++) {
				if ( ((*quote_vector)[i].year+1900==atoi(result_row2[2])) \
				   &&((*quote_vector)[i].month+1==atoi(result_row2[3])) \
				   &&((*quote_vector)[i].day==atoi(result_row2[4])) ) {

					(*quote_vector)[i].valid=TRUE;
					(*quote_vector)[i].timestamp=atol(result_row2[0]);
					(*quote_vector)[i].value=atof(result_row2[1]);
					if ((*quote_vector)[i].value>quote_maximum) quote_maximum=(*quote_vector)[i].value;
					break;
				}
			}
		}
		mysql_free_result(result_set2);

		quote_count_valid=0;
		quote_count_invalid=0;

		for (i=1;i<quote_count+1;i++) {
			if ((*quote_vector)[i].valid==TRUE) quote_count_valid++;
			else quote_count_invalid++;
		}
	}
			
	mysql_free_result(result_set);	
	g_free(stock_tname);
			
	interpolate_voids(*quote_vector,quote_count);

	return quote_count;
}

double	_press_x,_press_y;

gint on_chartcanvas_root_event(GnomeCanvasItem *item, GdkEvent *event, gpointer data) {
	char	status[200];
	double	percent;
	int	day;
	GdkCursor *fleur;
//	GtkAdjustment	*had,*vad;

	if (quote_vector==NULL) return FALSE;
	switch(event->type) {
		case GDK_MOTION_NOTIFY:
			day=rint(CHARTER_RX(event->motion.x))+1;
			if (day>0 && day <= quote_count) {
				percent=((quote_vector[1].value/quote_vector[day].value)-1)*100;
			
				sprintf(status,"%.2i.%.2i.%.4i %4.2f %+4.2f %%",quote_vector[day].day,quote_vector[day].month+1,quote_vector[day].year+1900,quote_vector[day].value,percent);
				gnome_appbar_set_status(chart_appbar,status);
				return TRUE;
			}
			if (event->motion.state && GDK_BUTTON2_MASK){
//				printf("Move\n");
/*				had=gtk_scrolled_window_get_hadjustment(chart_scrolled_window);
				vad=gtk_scrolled_window_get_vadjustment(chart_scrolled_window);
				gtk_adjustment_set_value(had,event->motion.x-_press_x);
				gtk_adjustment_set_value(vad,event->motion.y-_press_y);
//				gnome_canvas_item_move(GNOME_CANVAS_ITEM(item),event->motion.x-_press_x,event->motion.y-_press_y);
				printf("%f/%f\n",event->motion.x,event->motion.y);
				_press_x=event->motion.x;
				_press_y=event->motion.y;
	*/			
			}
			break;
			
		case GDK_BUTTON_PRESS:
			switch(event->button.button) {
				case 2:		// Middle Mousebutton
					fleur = gdk_cursor_new (GDK_FLEUR);
					_press_x=event->button.x;
					_press_y=event->button.y;
                                        
					gnome_canvas_item_grab (GNOME_CANVAS_ITEM(item),
						GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
						fleur, event->button.time);
					gdk_cursor_destroy (fleur);
			}
			break;
		case GDK_BUTTON_RELEASE:
			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM(item), event->button.time);
			break;

		default:break;
	}
	return FALSE;
}

volatile int	_cat_combo_block;	// Lock

void on_chart_cat_ce_changed(GtkEntry *entry) {
	if (_cat_combo_block) return;
	
	set_wait_cursor(GTK_WIDGET(chart_window),TRUE);
	init_stock_combo(gtk_entry_get_text(entry),NULL);
	set_wait_cursor(GTK_WIDGET(chart_window),FALSE);
}

volatile int	_stock_combo_block;	// Lock
char	*_last_stock;

void on_chart_stock_ce_changed(GtkEntry *entry) {
	
	if (strcmp(_last_stock,gtk_entry_get_text(entry))==0) return;

	if (strcmp("",gtk_entry_get_text(entry))==0) return;

	if (_stock_combo_block || _cat_combo_block) return;

	set_wait_cursor(GTK_WIDGET(chart_window),TRUE);

	g_free(_last_stock);
	_last_stock=strdup(gtk_entry_get_text(entry));

	quote_count=create_quote_vector(gtk_entry_get_text(entry),
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(chart_cat_combo)->entry)),
		&quote_vector);
	
	free_chart_instances();

	// The idea is the following: X axis scaling is the same, no matter how
	// much data there is. Y axis scaling is dynamic, such that you always
	// see the whole height of the chart.
	
	// FIXME: Add some clever code to have the chart fill the entire visible canvas
	
	pix_per_day=3.3;
	pix_per_eur=500/quote_maximum;

	// x=0,y=0 is in the lower right
	// First quote in list is newest value!!!

	c_x=0;
	c_y=0;
	c_x2=0;
	c_y2=0;

	g_list_foreach(type_list,&create_defaults,0);
	
	reorder_y_instances(chart_structure_ctree);

	adjust_scroll_region();
	
	update_global_conf_combo();
	update_local_conf_combo();

	set_wait_cursor(GTK_WIDGET(chart_window),FALSE);
}

void on_chartbutton_clicked(GtkWidget *widget, gpointer user_data) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	char	*stock_name;
	char	*cat_name;
	GtkCTreeNode *parent_node;
	GList	*categories_list;

	quote_maximum=0.0;

	if (selected_node!=NULL) {

		set_wait_cursor(GTK_WIDGET(chart_window),TRUE);
		if (GTK_CTREE_ROW(selected_node)->is_leaf) {
			_last_stock=strdup(" ");
		
			// get trading times from the parent category
			parent_node=GTK_CTREE_ROW(selected_node)->parent;
			cat_name=GTK_CELL_TEXT(GTK_CTREE_ROW(parent_node)->row.cell[0])->text;

			// init Category combo
			categories_list=NULL;
			sprintf(dbquery,"show tables like \'%s\'","\\_cat\\_%%"); //No other way
			mysql_query(mysql,dbquery);
			result_set=mysql_store_result(mysql);

			categories_list=g_list_append(categories_list,"All Stocks");
			while ((result_row=mysql_fetch_row(result_set))!=NULL) {
				categories_list=g_list_append(categories_list,get_category_name(result_row[0]));
			}

			_cat_combo_block=TRUE;
			_stock_combo_block=TRUE;

			gtk_combo_set_popdown_strings(chart_cat_combo,categories_list);
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(chart_cat_combo)->entry), cat_name);

			g_list_free(categories_list);
			mysql_free_result(result_set);	

			stock_name=GTK_CELL_TEXT(GTK_CTREE_ROW(selected_node)->row.cell[0])->text;

			gtk_widget_show(GTK_WIDGET(chart_window));

			init_stock_combo(cat_name,NULL);
			
			_stock_combo_block=FALSE;
			_cat_combo_block=FALSE;

			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(chart_stock_combo)->entry), stock_name);
		}
		set_wait_cursor(GTK_WIDGET(chart_window),FALSE);
	}
}

void adjust_scroll_region(void){
	gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(gnome_canvas_root(chart_canvas)),&c_x,&c_y,&c_x2,&c_y2);
	gnome_canvas_set_scroll_region(chart_canvas,c_x,c_y,c_x2,c_y2);
}

void Build_ChartElements_tree(GList *type_list) {
	
}

void set_name_callback(GtkWidget *widget,gpointer data) {
	char	*name=(char *)data;

	if (GTK_IS_LABEL(widget)) 
		gtk_label_set_text(GTK_LABEL(widget),name);
}

double	_running_y_offset;

void _reorder_move_xaxis(GtkCTree *tree,GtkCTreeNode *node, gpointer data) {
	ChartInstance *ci;
	ChartType *ct;
	double	x1,y1,x2,y2;

	ci=(ChartInstance *)gtk_ctree_node_get_row_data(tree,node);

	ct=ci->type;
	if (strcmp(ct->caps->type,"gchartman/x-axis")==0) {
		if (ci->move_group!=NULL) {
			gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(ci->move_group),
				&x1,&y1,&x2,&y2);
			
			gnome_canvas_item_move(GNOME_CANVAS_ITEM(ci->move_group),0,_running_y_offset-y1);
		}
	}
}

void _reorder_init_offsets(GtkCTree *tree,GtkCTreeNode *node, gpointer data) {
	ChartInstance *ci=(ChartInstance *)gtk_ctree_node_get_row_data(tree,node);

	if (ci->move_group) gnome_canvas_item_move(GNOME_CANVAS_ITEM(ci->move_group),0,-1.0*ci->y_offset);	
	ci->y_offset=0.0;
}

void _reorder_move_instances(GtkCTree *tree,GtkCTreeNode *node, gpointer data) {
	ChartInstance *ci=(ChartInstance *)gtk_ctree_node_get_row_data(tree,node);
	ChartInstance *ci_temp;
	ChartType *ct=ci->type;
	double	x1,y1,x2,y2,ofs;

	if (ct->caps->new_area) {
		if (ci->move_group!=NULL) {
			gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(ci->move_group),
				&x1,&y1,&x2,&y2);

			ofs=y2-y1;
			_running_y_offset+=ofs;
			ci->y_offset=_running_y_offset;

			gnome_canvas_item_move(GNOME_CANVAS_ITEM(ci->move_group),0,ci->y_offset);

		}
	} else {
		ci_temp=ci;
		if (ci_temp->parent!=NULL) {
			ci_temp=(ChartInstance *)ci_temp->parent;
			ci->y_offset=ci_temp->y_offset;
			if (ci->move_group) gnome_canvas_item_move(GNOME_CANVAS_ITEM(ci->move_group),0,ci_temp->y_offset);
		}
	}
}

void reorder_y_instances(GtkCTree *tree) {
	double	min_offset=0.0;

	_running_y_offset=0.0;

	gtk_ctree_pre_recursive(chart_structure_ctree,NULL,_reorder_init_offsets,&min_offset);
	gtk_ctree_pre_recursive(chart_structure_ctree,NULL,_reorder_move_instances,NULL);
	gtk_ctree_pre_recursive(chart_structure_ctree,NULL,_reorder_move_xaxis,NULL);
}

ChartInstance *add_chart_instance(ChartType *ct,GtkCTreeNode *parent,xmlNodePtr config_node,Quote *quote_vector,int quote_count) {
	ChartInstance *ci;
	char	*item_name[2];
	int	can_parent;

	if (ct==NULL) {
		printf("Invalid ChartType in add_chart_instance!!!\n");
		g_assert(ct!=NULL);
	}

	ci=(ChartInstance *)g_malloc(sizeof(ChartInstance));
	memset(ci,0,sizeof(ChartInstance));

	ci->type=ct;

	if (parent) {
		ci->parent=(ChartInstance *)gtk_ctree_node_get_row_data(chart_structure_ctree,parent);
	} else {
		ci->parent=NULL;
	}

	ci->quote_vector=malloc((quote_count+1)*sizeof(Quote));
	memcpy(ci->quote_vector,quote_vector,(quote_count+1)*sizeof(Quote));
	ci->quote_count=quote_count;

	ci->new_quote_vector=malloc((quote_count+1)*sizeof(Quote));
	memcpy(ci->new_quote_vector,quote_vector,(quote_count+1)*sizeof(Quote));
	ci->new_quote_count=quote_count;
	
	(ct->create)(ci,config_node);

	item_name[0]=strdup(ci->instance_name);	//Need to add some instance information
	item_name[1]=NULL;

	can_parent=ci->type->caps->parent;
	ci->tree_node=gtk_ctree_insert_node(chart_structure_ctree,parent,NULL,item_name,5,NULL,NULL,NULL,NULL,!can_parent,FALSE);

	gtk_ctree_node_set_row_data(chart_structure_ctree,ci->tree_node,ci);

	ci->y_offset=0;

	assert(ci!=NULL);
	
	return ci;
}

void create_from_toolbar(GtkWidget *widget, gpointer user_data) {
	ChartType *ct=(ChartType *)user_data;
	int	valid=TRUE;
	ChartInstance *ci,*ci_p;
	GtkCTreeNode *parent;
	Quote	*quotev;
	int	quotec;

	if (ct->caps->child) {
		if (selected_instance_node==NULL) valid=FALSE;
		else {
			ci=(ChartInstance *)gtk_ctree_node_get_row_data(chart_structure_ctree,selected_instance_node);
			if (!ci->type->caps->parent) valid=FALSE;
		}
		if (valid==FALSE) {
			error_dialog("Please select a parent item from the chart structure and retry");
			return;
		}
		parent=selected_instance_node;
	} else {
		parent=NULL;
	}

	if (parent) {
		ci_p=(ChartInstance *)gtk_ctree_node_get_row_data(chart_structure_ctree,parent);
		quotev=ci_p->new_quote_vector;
		quotec=ci_p->new_quote_count;
	} else {
		quotev=quote_vector;
		quotec=quote_count;
		ci_p=NULL;
	}
	
	ci=add_chart_instance(ct,parent,NULL,quotev,quotec);
	ci->parent=ci_p;

	reorder_y_instances(chart_structure_ctree);

	adjust_scroll_region();
}

void on_instance_properties_button_clicked(GtkWidget *widget, gpointer user_data) {
	ChartInstance	*ci,*p_ci;
	Quote	*qv;
	int	*qc,qc2;
	char	*new_name;

	// FIXME:
	printf("FIXME: recursively recreate children\n");

	ci=(ChartInstance *)gtk_ctree_node_get_row_data(chart_structure_ctree,selected_instance_node);

	if (GTK_CTREE_ROW(selected_instance_node)->parent) {
		p_ci=(ChartInstance *)gtk_ctree_node_get_row_data(chart_structure_ctree,GTK_CTREE_ROW(selected_instance_node)->parent);
		qv=p_ci->new_quote_vector;
		qc=&p_ci->new_quote_count;
	} else {
		qv=quote_vector;
		qc2=quote_count;
		qc=&qc2;
	}

	free(ci->quote_vector);

	ci->quote_vector=malloc((*qc+1)*sizeof(Quote));
	memcpy(ci->quote_vector,qv,(*qc+1)*sizeof(Quote));
	ci->quote_count=*qc;

	(ci->type->set_properties)(ci);
	
	new_name=ci->instance_name;

	gtk_ctree_node_set_text(chart_structure_ctree,ci->tree_node,0,new_name);

//	ci->y_offset=0.0;	This is now done by modules when they need it!

	reorder_y_instances(chart_structure_ctree);

	adjust_scroll_region();
}

void on_instance_delete_button_clicked(GtkWidget *widget, gpointer user_data) {
	ChartInstance *ci;
	
	ci=(ChartInstance *)gtk_ctree_node_get_row_data(chart_structure_ctree,selected_instance_node);

	if (GTK_CTREE_ROW(selected_instance_node)->children!=NULL) {
		error_dialog("Please delete any items depending on this one first\nFIXME: add recursive deletion & maybe warn if deleting a whole tree");
		return;
	}

	free_ctree_item(chart_structure_ctree,selected_instance_node,NULL);

	reorder_y_instances(chart_structure_ctree);
	
	adjust_scroll_region();
}

void on_chart_structure_ctree_select_row(GtkCTree *ctree, GtkCTreeNode *row, gint column) {
	ChartInstance	*ci;

	selected_instance_node=row;
	ci=(ChartInstance *)gtk_ctree_node_get_row_data(ctree,row);
	if (ci->type->caps->deleteable) gtk_widget_set_sensitive(GTK_WIDGET(instance_delete_button),TRUE);
	else gtk_widget_set_sensitive(GTK_WIDGET(instance_delete_button),FALSE);
	
	if (ci->type->set_properties) gtk_widget_set_sensitive(GTK_WIDGET(instance_properties_button),TRUE);
	else gtk_widget_set_sensitive(GTK_WIDGET(instance_properties_button),FALSE);
}

void on_chart_structure_ctree_unselect_row(GtkCTree *ctree, GtkCTreeNode *row, gint column) {

	selected_instance_node=NULL;
	gtk_widget_set_sensitive(GTK_WIDGET(instance_delete_button),FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(instance_properties_button),FALSE);
}

void on_edit_comment_dialog_show(GtkWidget *widget, gpointer user_data) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	char	*stock_name;
	char	e_stock_name[200];

//	gtk_editable_delete_text(GTK_EDITABLE(comment_textbox),0,comment_textbox->text_len);

	gtk_text_set_point(comment_textbox,0);
	gtk_text_forward_delete(comment_textbox,gtk_text_get_length(comment_textbox));

	stock_name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(chart_stock_combo)->entry));
	mysql_escape_string(e_stock_name,stock_name,strlen(stock_name));
	sprintf(dbquery,"select Comment from _TableNames where Name='%s'",e_stock_name);
	
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);

	if ((result_row=mysql_fetch_row(result_set))!=NULL) {
		gtk_text_insert(comment_textbox,NULL,NULL,NULL,result_row[0],-1);
	}
	mysql_free_result(result_set);
}

char	*_edit_comment_comment;
char	*_edit_comment_stockname;

void _update_comment(GtkCTree *ctree,GtkCTreeNode *node,gpointer user_data) {
	char	*node_name;

	node_name=GTK_CELL_TEXT(GTK_CTREE_ROW(node)->row.cell[0])->text;
//	gtk_ctree_node_get_text(ctree,node,0,&nodename);
	if (strcmp(node_name,_edit_comment_stockname)==0)
		gtk_ctree_node_set_text(ctree,node,2,_edit_comment_comment);
}

void on_edit_comment_dialog_clicked(GnomeDialog *dialog, gint button_number){

	// FIXME: dbquery needs to be dynamic
	char	dbquery[1000];
	char	*stock_name;
	char	e_stock_name[200];
	char	*e_comment;
	char	*comment;

	if (button_number==0) {
		e_comment=alloca(2*gtk_text_get_length(comment_textbox)+2);
		comment=gtk_editable_get_chars(GTK_EDITABLE(comment_textbox),0,-1);
		mysql_escape_string(e_comment,comment,gtk_text_get_length(comment_textbox));
	
		stock_name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(chart_stock_combo)->entry));
		mysql_escape_string(e_stock_name,stock_name,strlen(stock_name));
		sprintf(dbquery,"update _TableNames set Comment='%s' where Name='%s'",e_comment,e_stock_name);

		mysql_query(mysql,dbquery);
		
		// quickly (?) update comment in table_tree

		_edit_comment_comment=comment;
		_edit_comment_stockname=stock_name;
		gtk_ctree_post_recursive(table_tree,NULL,_update_comment,NULL);
	}
}

void on_chart_editcomment_clicked(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_run(edit_comment_dialog);
	gtk_widget_hide(GTK_WIDGET(edit_comment_dialog));
}

void on_split_stock_dialog_show(GtkWidget *widget, gpointer user_data) {
	gnome_date_edit_set_time(split_stock_date_edit,time(NULL));
}

void on_split_stock_dialog_clicked(GnomeDialog *dialog, gint button_number){
	// FIXME: dbquery needs to be dynamic
	char	dbquery[1000];
	char	date_string[30];
	char	*stock_tname;
	char	e_stock_tname[200];
	time_t	time;
	struct tm *time_tm;
	gfloat	upper,lower;

	if (button_number==0) {
		time=gnome_date_edit_get_date(split_stock_date_edit);
		time_tm=localtime(&time);
		time_tm->tm_hour=23;
		time_tm->tm_min=59;
		time_tm->tm_sec=59;

		sprintf(date_string,"%.4i-%.2i-%.2i %.2i:%.2i:%.2i",time_tm->tm_year+1900,
                                           time_tm->tm_mon+1,
                                           time_tm->tm_mday,
                                           time_tm->tm_hour,
                                           time_tm->tm_min,
                                           time_tm->tm_sec);

		upper=gtk_spin_button_get_value_as_float(split_stock_upper_spin);
		lower=gtk_spin_button_get_value_as_float(split_stock_lower_spin);

		stock_tname=get_stock_tname(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(chart_stock_combo)->entry)),NULL);
		mysql_escape_string(e_stock_tname,stock_tname,strlen(stock_tname));
		sprintf(dbquery,"update %s set value=value*(%f/%f),time=time where time <= \'%s\'",e_stock_tname,upper,lower,date_string);

		mysql_query(mysql,dbquery);

		if (_last_stock!=NULL) g_free(_last_stock);
		_last_stock=strdup("!");	// anything will do

		on_chart_stock_ce_changed(GTK_ENTRY(GTK_COMBO(chart_stock_combo)->entry));
		
	}
}

void on_chart_splitstock_clicked(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_run(split_stock_dialog);
	gtk_widget_hide(GTK_WIDGET(split_stock_dialog));
}

void on_chartwindow_show(GtkWidget *widget, gpointer user_data) {

}

void free_charttype (gpointer node_data, gpointer user_data) {
	ChartType *ct=(ChartType *)node_data;

	dlclose(ct->handle);
	g_free(ct);
}

gint on_chartwindow_delete_event(GtkWidget *widget, gpointer user_data) {
	// Clean everything up here
	free_chart_instances();

	gtk_clist_clear(GTK_CLIST(chart_structure_ctree));
	g_free(quote_vector);
	quote_vector=NULL;
	quote_count=0;

	free(_selected_table_name);

	gtk_widget_hide(widget);
	return TRUE;
}

void redraw_chart (GtkCTree *tree,GtkCTreeNode *node,gpointer data) {
	ChartInstance *ci,*p_ci;
	ChartType *ct;
	Quote	*qv,*qv2;
	int	qc,qc2;

	ci=(ChartInstance *)gtk_ctree_node_get_row_data(tree,node);
	
	ct=ci->type;

	if (GTK_CTREE_ROW(node)->parent) {
		p_ci=(ChartInstance *)gtk_ctree_node_get_row_data(tree,GTK_CTREE_ROW(node)->parent);
		qv=p_ci->quote_vector;
		qc=p_ci->quote_count;
	} else {
		qv=quote_vector;
		qc=quote_count;
	}

	// create throw-away working copy

	qv2=malloc((qc+1)*sizeof(Quote));
	memcpy(qv2,qv,(qc+1)*sizeof(Quote));
	qc2=qc;
	
	ct->redraw(ci);
	
	free(qv2);

//	ct->rezoom(ci->instance_data,ci->quote_vector,ci->quote_count);

	ci->y_offset=0.0;
}

void chart_redraw(GtkCTree *tree) {

	gtk_ctree_pre_recursive(tree,NULL,redraw_chart,NULL);

	reorder_y_instances(tree);

	adjust_scroll_region();
}

gint on_chart_xscaleplus_event(GtkWidget *item, GdkEvent *event, gpointer data) {
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
			set_wait_cursor(GTK_WIDGET(chart_window),TRUE);
			if (event->button.button==1) pix_per_day*=1.1;	// Left
			if (event->button.button==2) pix_per_day*=1.4641;	// Middle
			if (event->button.button==3) pix_per_day*=1.9487171;	// Right
			c_x=0; c_y=0; c_x2=0; c_y2=0;
			chart_redraw(chart_structure_ctree);
			set_wait_cursor(GTK_WIDGET(chart_window),FALSE);

			break;
		default:break;
	}
	return FALSE;
}

gint on_chart_xscaleminus_event(GtkWidget *item, GdkEvent *event, gpointer data) {
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
			set_wait_cursor(GTK_WIDGET(chart_window),TRUE);
			if (event->button.button==1) pix_per_day/=1.1;	// Left
			if (event->button.button==2) pix_per_day/=1.4641;	// Middle
			if (event->button.button==3) pix_per_day/=1.9487171;	// Right
			c_x=0; c_y=0; c_x2=0; c_y2=0;
			chart_redraw(chart_structure_ctree);
			set_wait_cursor(GTK_WIDGET(chart_window),FALSE);

			break;
		default:break;
	}
	return FALSE;
}

gint on_chart_yscaleplus_event(GtkWidget *item, GdkEvent *event, gpointer data) {
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
			set_wait_cursor(GTK_WIDGET(chart_window),TRUE);
			if (event->button.button==1) pix_per_eur*=1.1;	// Left
			if (event->button.button==2) pix_per_eur*=1.4641;	// Middle
			if (event->button.button==3) pix_per_eur*=1.9487171;	// Right
			c_x=0; c_y=0; c_x2=0; c_y2=0;
			chart_redraw(chart_structure_ctree);
			set_wait_cursor(GTK_WIDGET(chart_window),FALSE);

			break;
		default:break;
	}
	return FALSE;
}

gint on_chart_yscaleminus_event(GtkWidget *item, GdkEvent *event, gpointer data) {
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
			set_wait_cursor(GTK_WIDGET(chart_window),TRUE);
			if (event->button.button==1) pix_per_eur/=1.1;	// Left
			if (event->button.button==2) pix_per_eur/=1.4641;	// Middle
			if (event->button.button==3) pix_per_eur/=1.9487171;	// Right
			c_x=0; c_y=0; c_x2=0; c_y2=0;
			chart_redraw(chart_structure_ctree);
			set_wait_cursor(GTK_WIDGET(chart_window),FALSE);

			break;
		default:break;
	}
	return FALSE;
}

void update_global_conf_combo(void) {
	GList	*glist=NULL;
	int	count=0;
	xmlNodePtr node;

	gtk_list_clear_items(GTK_LIST(GTK_COMBO(global_config_name_combo)->list),0,-1);

	node=db_global_configs_node->children;
	if (node) {
		while (node) {
			if (strcmp(node->name,"config")==0) {
				glist=g_list_append(glist,xmlGetProp(node,"name"));
				count++;
			}
			node=node->next;
		}
	}

	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(global_config_name_combo)->entry),"");
	if (count>0) {
		gtk_combo_set_popdown_strings(global_config_name_combo, glist);
	}
}

void update_local_conf_combo(void) {
	xmlNodePtr node=db_local_configs_node;
	int	found=FALSE;
	char	*name;
	int	count=0;
	GList	*glist=NULL;
	
	name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(chart_stock_combo)->entry));
	node=db_local_configs_node->children;
	
	while (node) {
		if (strcmp(node->name,"stock")==0) {
			if (strcmp(xmlGetProp(node,"name"),name)==0) {
				found=TRUE;
				break;
			}
		}
		node=node->next;
	}

	gtk_list_clear_items(GTK_LIST(GTK_COMBO(local_config_name_combo)->list),0,-1);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(local_config_name_combo)->entry),"");

	if (found==TRUE) {
		// iterate through <config> and add to combo
		stock_local_configs_node=node;
		node=node->children;
		while (node) {
			if (strcmp(node->name,"config")==0) {
				glist=g_list_append(glist,xmlGetProp(node,"name"));
				count++;
			}
			node=node->next;
		}
		if (count>0) {
			gtk_combo_set_popdown_strings(local_config_name_combo, glist);
		}
	} else {
		node=xmlNewNode(NULL,"stock");
		xmlNewProp(node,"name",xmlEncodeEntitiesReentrant(local_configs_doc,name));
		xmlAddChild(db_local_configs_node,node);
		stock_local_configs_node=node;
	}
}

/*
 *	These global variables configure the function below. It should be called by
 *	g_list_foreach.
 */


char	*_save_instance_tname;	//table name
char	*_save_instance_cname;	//config name

void save_chart_instance(GtkCTree *tree, GtkCTreeNode *node, gpointer data) {
	ChartInstance *ci;
	ChartType *ct;
	xmlNodePtr parent_node=(xmlNodePtr)data;
	
/*	if (GTK_CTREE_ROW(node)->parent!=NULL) {
		ci=(ChartInstance *)gtk_ctree_node_get_row_data(tree,GTK_CTREE_ROW(node)->parent);
//		if (ci->xml_node!=NULL) parent_node=ci->xml_node;
		if (ci->save_node!=NULL) parent_node=ci->save_node;
	}*/
	
	ci=(ChartInstance *)gtk_ctree_node_get_row_data(tree,node);
	
	ct=ci->type;

	if (ci->parent) {
		parent_node=ci->parent->save_node;
	}
	
	if (ci->xml_node!=NULL) {
		ci->save_node=xmlCopyNode(ci->xml_node,1);
		xmlAddChild(parent_node,ci->save_node);

/*		node_copy=xmlCopyNode(ci->xml_node,1);
		xmlAddChild(parent_node,node_copy);
		xmlFreeNode(ci->xml_node);
		ci->xml_node=node_copy;*/
	}
}

ChartType *find_charttype(char *type) {
	GList	*list;
	ChartType *ct;
	ChartType *retval;
	char	*t;

	// FIXME: put back the nice hashtable to handle this stuff

	t=g_strdup(type);
	retval=NULL;	
	for (list=g_list_first(type_list); list!=NULL; list=g_list_next(list)) {
		ct=(ChartType *)list->data;
		if (strcmp(ct->caps->type,t)==0) {
			retval=(ChartType *)(list->data);
			break;
		}
	}
	g_free(t);
	
	return retval;
}

char	*_cnd_put_name;
int	_cnd_local;

void on_getconfignamedialog_show(GtkWidget *widget, gpointer user_data) {
	gtk_entry_set_text(GTK_ENTRY(new_config_name_entry),_cnd_put_name);
	if (_cnd_local) {
		gtk_widget_set_sensitive(new_config_pos_button,TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(new_config_pos_button),FALSE);
		gtk_widget_set_sensitive(new_config_pos_button,FALSE);
	}

}

/*	To iterate is human,
 *	  to recurse divine.
 *	   -L. Peter Deutsch
 */
 
void recursive_recreate_instances(ChartInstance *parent_ci,xmlNodePtr node,GtkCTreeNode *ctree_parent_node,Quote *quotev,int quotec) {
	GtkAdjustment *had,*vad;
	ChartType *ct;
	ChartInstance *ci;
	int	recreate_children=TRUE;

	if (strcmp(node->name,"item")==0) {
		if (strcmp(xmlGetProp(node,"type"),"gchartman-internal/zoom_settings")==0) {
			had=gtk_scrolled_window_get_hadjustment(chart_scrolled_window);
			vad=gtk_scrolled_window_get_vadjustment(chart_scrolled_window);

			pix_per_eur=atof(xmlGetProp(node,"pix_per_eur"));
			pix_per_day=atof(xmlGetProp(node,"pix_per_day"));

			// FIXME: Do this somewhere else
			gtk_adjustment_set_value(had,atof(xmlGetProp(node,"hpos")));
			gtk_adjustment_set_value(vad,atof(xmlGetProp(node,"vpos")));
		} else {
			// FIXME: create new quote_vectors and save in ChartInstance
			ct=find_charttype(xmlGetProp(node,"type"));
			if (ct) {
				ci=add_chart_instance(ct,ctree_parent_node,node,quotev,quotec);
				ci->parent=parent_ci;
			} else recreate_children=FALSE;
		}
	}
	
	if (node->children && recreate_children==TRUE) recursive_recreate_instances(ci,node->children,ci->tree_node,ci->new_quote_vector,ci->new_quote_count);
	if (node->next) recursive_recreate_instances(parent_ci,node->next,ctree_parent_node,quotev,quotec);	
}

void on_get_global_config_clicked(GtkWidget *widget, gpointer user_data) {
	char	*name;
	xmlNodePtr node;

	name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(global_config_name_combo)->entry));
	if (strcmp(name," ")==0) return;
	
	// Delete old stuff

	free_chart_instances();
	gtk_clist_clear(GTK_CLIST(chart_structure_ctree));

	node=db_global_configs_node->children;
	
	while (node) {
		if (strcmp(node->name,"config")==0) {
			if (strcmp(xmlGetProp(node,"name"),name)==0) {
				break;
			}
		}
		node=node->next;
	}

	recursive_recreate_instances(NULL,node->children,NULL,quote_vector,quote_count);

	reorder_y_instances(chart_structure_ctree);

	adjust_scroll_region();
}

void on_put_global_config_clicked(GtkWidget *widget, gpointer user_data) {
	char	*name;
	xmlNodePtr config_node,node;

	_cnd_put_name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(global_config_name_combo)->entry));
	_cnd_local=FALSE;

	if (gnome_dialog_run(get_config_name_dialog)==0) {
		name=gtk_entry_get_text(GTK_ENTRY(new_config_name_entry));

		// Remove existing node
		node=db_global_configs_node->children;
		while (node) {
			if (strcmp(node->name,"config")==0) {
				if (strcmp(xmlGetProp(node,"name"),name)==0) {
					xmlUnlinkNode(node);
					xmlFreeNode(node);
					break;
				}
			}
			node=node->next;
		}

		config_node=xmlNewNode(NULL,"config");
		xmlNewProp(config_node,"name",xmlEncodeEntitiesReentrant(global_configs_doc,name));
		xmlAddChild(db_global_configs_node,config_node);

		gtk_ctree_pre_recursive(chart_structure_ctree,NULL,save_chart_instance,config_node);
	} else {
	}

	gtk_widget_hide(GTK_WIDGET(get_config_name_dialog));

	xmlSaveFile(global_configs_filename,global_configs_doc);
	update_global_conf_combo();
}

void on_del_global_config_clicked(GtkWidget *widget, gpointer user_data) {
	char	*name;
	xmlNodePtr node;
	
	name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(global_config_name_combo)->entry));

	node=db_global_configs_node->children;
	while (node) {
		if (strcmp(node->name,"config")==0) {
			if (strcmp(xmlGetProp(node,"name"),name)==0) {
				xmlUnlinkNode(node);
				xmlFreeNode(node);
				break;
			}
		}
		node=node->next;
	}

	update_global_conf_combo();
}

char	*zoom_settings_e;

void on_get_local_config_clicked(GtkWidget *widget, gpointer user_data) {
	char	*name;
	xmlNodePtr node;

	name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(local_config_name_combo)->entry));
	if (strcmp(name," ")==0) return;
	
	// Delete old stuff

	free_chart_instances();
	gtk_clist_clear(GTK_CLIST(chart_structure_ctree));

	node=stock_local_configs_node->children;
	
	while (node) {
		if (strcmp(node->name,"config")==0) {
			if (strcmp(xmlGetProp(node,"name"),name)==0) {
				break;
			}
		}
		node=node->next;
	}

/*	printf("recreating from:\n");
	xmlElemDump(0,NULL,node);
	printf("\n");
*/	recursive_recreate_instances(NULL,node->children,NULL,quote_vector,quote_count);

	reorder_y_instances(chart_structure_ctree);

	adjust_scroll_region();
}

void on_put_local_config_clicked(GtkWidget *widget, gpointer user_data) {
	GtkAdjustment *had,*vad;
	char	*name;
	xmlNodePtr config_node,node,new_node;
	char	temp_string[20];

	_cnd_put_name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(local_config_name_combo)->entry));
	_cnd_local=TRUE;

	if (gnome_dialog_run(get_config_name_dialog)==0) {
		name=gtk_entry_get_text(GTK_ENTRY(new_config_name_entry));

		// Remove existing node
		node=stock_local_configs_node->children;
		while (node) {
			if (strcmp(node->name,"config")==0) {
				if (strcmp(xmlGetProp(node,"name"),name)==0) {
					xmlUnlinkNode(node);
					xmlFreeNode(node);
					break;
				}
			}
			node=node->next;
		}

		config_node=xmlNewNode(NULL,"config");
		xmlNewProp(config_node,"name",xmlEncodeEntitiesReentrant(local_configs_doc,name));
		xmlAddChild(stock_local_configs_node,config_node);

		if (GTK_TOGGLE_BUTTON(new_config_pos_button)->active) {
			// save pix_per_eur, pix_per_day & scrolledwindow adjustments

			had=gtk_scrolled_window_get_hadjustment(chart_scrolled_window);
			vad=gtk_scrolled_window_get_vadjustment(chart_scrolled_window);

			new_node=xmlNewNode(NULL,"item");
			xmlNewProp(new_node,"type","gchartman-internal/zoom_settings");
			sprintf(temp_string,"%e",pix_per_eur);
			xmlNewProp(new_node,"pix_per_eur",temp_string);
			sprintf(temp_string,"%e",pix_per_day);
			xmlNewProp(new_node,"pix_per_day",temp_string);
			sprintf(temp_string,"%e",GTK_ADJUSTMENT(had)->value);
			xmlNewProp(new_node,"hpos",temp_string);
			sprintf(temp_string,"%e",GTK_ADJUSTMENT(vad)->value);
			xmlNewProp(new_node,"vpos",temp_string);
			xmlAddChild(config_node,new_node);

		}
		gtk_ctree_pre_recursive(chart_structure_ctree,NULL,save_chart_instance,config_node);
/*		printf("saving to:\n");
		xmlElemDump(0,NULL,config_node);
		printf("\n");
*/	} else {
	}

	gtk_widget_hide(GTK_WIDGET(get_config_name_dialog));

	xmlSaveFile(local_configs_filename,local_configs_doc);
	update_local_conf_combo();
}

void on_del_local_config_clicked(GtkWidget *widget, gpointer user_data) {
	char	*name;
	xmlNodePtr node;
	
	name=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(local_config_name_combo)->entry));

	node=stock_local_configs_node->children;
	while (node) {
		if (strcmp(node->name,"config")==0) {
			if (strcmp(xmlGetProp(node,"name"),name)==0) {
				xmlUnlinkNode(node);
				xmlFreeNode(node);
				break;
			}
		}
		node=node->next;
	}

	update_local_conf_combo();
}

