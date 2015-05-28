#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <glob.h>
#include <assert.h>
#include "gchartman.h"
#include "getvtquotes.h"
#include "connectdb.h"

//----------------------------------------------------------------------------------
//---- Use this to define wether alevt-cap is called with the `-oldbttv' option ----
#define OLDBTTV
//#undef OLDBTTV
//----------------------------------------------------------------------------------

#define VT_TIMEOUT 1200.0	// 20 Minutes
#define UPDATES_PER_SEC 15
#define VT_TIMEOUT_DELAY (1000000/UPDATES_PER_SEC)
#define VT_TIMEOUT_INC (100.0/(VT_TIMEOUT*UPDATES_PER_SEC)) // 100.0 = maximum

/*
 *	Global stuff. "displayed_page" points to the page actually displayed.
 */

	VtPage	*displayed_page=NULL;
	char	displayed_page_num_text[10];
	int	actual_selection_type=0;
	int	child_error;
	pid_t	cpid;
	int	row;

/*
 *	This function sets the sensitivity of the Left/Right/Up/Down/Taller/Smaller
 *	buttons.
 *	In:	integers,one for each button.
 */

void buttons_sens(gint up,gint down,gint left,gint right,gint taller,gint smaller,gint dis) {
	gtk_widget_set_sensitive((GtkWidget *)vt_up,up);
	gtk_widget_set_sensitive((GtkWidget *)vt_down,down);
	gtk_widget_set_sensitive((GtkWidget *)vt_left,left);
	gtk_widget_set_sensitive((GtkWidget *)vt_right,right);
	gtk_widget_set_sensitive((GtkWidget *)vt_taller,taller);
	gtk_widget_set_sensitive((GtkWidget *)vt_smaller,smaller);
	gtk_widget_set_sensitive((GtkWidget *)disable_button,dis);
}

/*
 *	This function initializes the "page_selections" of new pages.
 *	In:	page:	pointer to the page to be initialized
 */

void init_selections(VtPage *page) {
	int	i;

	assert(page!=NULL);
	
	for (i=0;i<NUM_SELS;i++) {
		page->page_selections[i].c1=0.0;
		page->page_selections[i].c2=0.0+xstep;
	}

	page->page_selections[SEL_TOPBOTTOM].c1=0.0;
	page->page_selections[SEL_TOPBOTTOM].c2=0.0+ystep;
	
	sprintf(page->category,"%s","[none]");
}

/*
 *	This function deletes the linked list of pages and frees their
 *	memory.
 *	In:	page:	pointer to the header of the list
 */

void erase_list(VtPage *page) {

	assert(page!=NULL);

	while (page->next!=NULL) {
		page=page->next;
	}
	
	while ((page->previous!=&vtpage_header)&&(page->previous!=NULL)) {
		page=page->previous;
		free(page->next);
	}
	vtpage_header.next=NULL;
	
}

/*
 *	This function seeks to the end of the linked list of pages and
 *	appends one page to it.
 *	In:	page:	pointer to the head of the list
 *	Out:	returns a pointer to the newly allocated page
 */

VtPage *append_vtpage(VtPage *page) {
	VtPage *temp;

	assert(page!=NULL);

	while (page->next!=NULL) {
		page=page->next;
	}

	page->next=malloc(sizeof(VtPage));
	memset(page->next,0,sizeof(VtPage));
	temp=page;
	page=page->next;		//vtpages points to the new page
	page->next=NULL;
	page->previous=temp;	//The new page is now set up
	
	return page;
}

/*	This function finds the last subpage with the same
 *	row_number as the given page. Note that subpages are 
 *	(and have to be) "in order". It is needed to rebuild
 *	the pagelist.
 *	In:	page:	pointer to any subpage with the row_number
 *			that you want to search for
 *	Out:	returns a pointer to the last Page found with the
 *		row_number of the given page
 */

VtPage *find_end_of_subpage(VtPage *page) {
	int	our_row;
	int	break_it;

	assert(page!=NULL);
	
	our_row=page->row_number;
	break_it=FALSE;
	while (break_it==FALSE) {
		if (page->next!=NULL) {
			if (page->next->row_number==our_row) {
				page=page->next;
			} else {
				break_it=TRUE;
			}
		} else {
			break_it=TRUE;
		}
	}
	return page;
}

/*
 *	This function rebuilds the pagelist entirely from the
 *	linked list of pages.
 *	In:	page:	pointer to the head of the list of pages
 *		List:	pointer to the gtk-clist widget
 */

void rebuild_pagelist(VtPage *page,GtkCList *clist) {
	char	page_d[4][50];	// 3 lines a 50 chars
	char	*page_data[4];
	int	i;

	assert(page!=NULL);
	assert(clist!=NULL);

	for (i=0;i<3;i++)
	{
		page_data[i]=page_d[i];
	}

	gtk_clist_clear(clist);

	while (page->next!=NULL)
	{
		page=page->next;

		if (page->sub_page_number==-1)
		{
			sprintf(page_data[0],"%i",page->page_number);
			sprintf(page_data[1],"%s","-");
			sprintf(page_data[2],"%s","-");
			page_data[3]=NULL;
			gtk_clist_append(clist,page_data);
		} else
		{
			sprintf(page_data[0],"%i",page->page_number);
			sprintf(page_data[1],"%i",page->sub_page_number);
			
			page=find_end_of_subpage(page);
			
			sprintf(page_data[2],"%i",page->sub_page_number);
			page_data[3]=NULL;
			gtk_clist_append(clist,page_data);
		}
	}
}

/*
 *	This function should be called after changing "displayed_page".
 *	It updates the UI with the values from the page.
 *	In:	page:	pointer to the page to be displayed.
 */

void display_this_page(VtPage *page) {
	int	itemp,i;

	if (page!=displayed_page) {

		displayed_page=page;

		gtk_widget_realize(GTK_WIDGET(vt_select_area));

		if (page==NULL) {
			gnome_canvas_item_set(vtsa_text,"text","No page loaded.",NULL);
		} else {	
			gnome_canvas_item_set(vtsa_text,"text",page->page_text,NULL);
			if (page->sub_page_number==-1) {
				// Single Page
				sprintf(displayed_page_num_text,"%i",page->page_number);
			} else {
				// Multi Page
				sprintf(displayed_page_num_text,"%i.%i",page->page_number,page->sub_page_number);
			}
			gtk_label_set_text(vtpage_displayed_label,displayed_page_num_text);
			itemp=actual_selection_type;
		
			for (i=0;i<NUM_SELS;i++) {
				actual_selection_type=i;
				update_selection();
			}
			actual_selection_type=itemp;
		
			gtk_entry_set_text(GTK_ENTRY(select_category_combo->entry),page->category);
		}

		gnome_canvas_update_now(vt_select_area);
		while (g_main_iteration(FALSE));
	}
}

/*
 *	This function is called when the VT-Window pops up. It reads
 *	the linked list of pages from the database, rebuilds the pagelist
 *	from it and displays the first page in the UI.
 *	In:	Std. Gtk+ Calback.
 */

void on_vtwin_show(GtkWidget *widget, gpointer user_data) {
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	GList	*combo_strings=NULL;
	gchar	dbquery[200];

/*	buttons_sens(FALSE,FALSE,TRUE,TRUE,TRUE,TRUE,FALSE);

	actual_selection_type=SEL_NAME;
	update_selection();
*/
	if (mysql!=NULL) {

		// Initialize the Category combo:
		combo_strings=g_list_append(combo_strings,"[none]");
		sprintf(dbquery,"show tables like \"\\_cat\\_%%\"");
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);
		if (mysql_errno(mysql)==0) {
			while ((result_row=mysql_fetch_row(result_set))!=NULL) {
				combo_strings=g_list_append(combo_strings,get_category_name(result_row[0]));
			}
		}
		mysql_free_result(result_set);

		gtk_combo_set_popdown_strings(select_category_combo, combo_strings);

		vtpages=&vtpage_header;		
		erase_list(vtpages);
		vtpages=&vtpage_header;		

		// Get Data from DB here and initialize the vtpage list

		sprintf(dbquery,"select row,pagenum,subpagenum,filename,selection,Category from _vtsettings order by row,pagenum,subpagenum");		
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);
		
		if (mysql_errno(mysql)==0) {
			while ((result_row=mysql_fetch_row(result_set))!=NULL) {
				vtpages=append_vtpage(vtpages);
				init_selections(vtpages);			
				vtpages->row_number=atoi(result_row[0]);
				vtpages->page_number=atoi(result_row[1]);
				vtpages->sub_page_number=atoi(result_row[2]);
				strcpy(vtpages->filename,result_row[3]);
				memcpy(vtpages->page_selections,result_row[4],sizeof(vtpages->page_selections));
				strcpy(vtpages->category,result_row[5]);
			}
		}
		mysql_free_result(result_set);
		
		rebuild_pagelist(&vtpage_header,page_list);
		
		displayed_page=(VtPage *)-1;	// Clear the canvas
		display_this_page(NULL);
		
/*		if (vtpage_header.next!=NULL) {
			display_this_page(vtpage_header.next);
		} else {
			display_this_page(NULL);
		}
*/	
	} else {
		error_dialog("You need to connect to a database first.");
		gtk_widget_hide(vt_box);
	}
}

/*
 *	This function saves all the data of a page to the database, except
 *	for the actual text.
 *	In:	my_sql:	pointer to the MYSQL object to use
 *		vtpages:pointer to the page to save
 */

void add_vt_settings_to_db(MYSQL *my_sql,VtPage *vtpages) {
	// Ugly:
	gchar	dbquery[200+(2*sizeof(vtpages->page_selections)+1)];
	gchar	escaped_sels[2*sizeof(vtpages->page_selections)+1];

	assert(vtpages!=NULL);
	assert(my_sql!=NULL);

	mysql_escape_string(escaped_sels,(char *)vtpages->page_selections,sizeof(vtpages->page_selections));
	
	sprintf(dbquery,"insert into _vtsettings(row,pagenum,subpagenum,filename,selection,Category) values(%i,%i,%i,'%s','%s','%s')",
		vtpages->row_number,
		vtpages->page_number,
		vtpages->sub_page_number,
		vtpages->filename,
		escaped_sels,
		vtpages->category);

//	g_message("%s\n",dbquery);

//	g_message("%i,%i",sizeof(escaped_sels),sizeof(vtpages->page_selections));
	
	mysql_real_query(my_sql,dbquery,sizeof(dbquery));
}

/*
 *	These functions calculate different (Text-)coordinates from
 *	a VTSelection structure.
 *	In:	VTSel:	pointer to the VTSelection to get the value from
 *	Out:	returns the value :-)
 */

int get_num_rows(VtSelection *vt_sel) {
	if (vt_sel->disabled==TRUE) {return 0;}
	
	return (int)(((vt_sel->c2)-(vt_sel->c1))/ystep);
}

int get_num_cols(VtSelection *vt_sel) {
	if (vt_sel->disabled==TRUE) {return 0;}
	
	return (int)(((vt_sel->c2)-(vt_sel->c1))/xstep);
}

int get_x_start(VtSelection *vt_sel) {
	return (int)((vt_sel->c1)/(xstep));
}

int get_x_end(VtSelection *vt_sel) {
	return (int)((vt_sel->c2)/(xstep));
}

int get_y_start(VtSelection *vt_sel) {
	return (int)((vt_sel->c1)/(ystep));
}

int get_y_end(VtSelection *vt_sel) {
	return (int)((vt_sel->c2)/(ystep));
}

/*
 *	This function extracts a text string from a videotext page.
 *	It skips leading and trailing spaces, preserving spaces
 *	between words.
 *	In:	my_x:	x coordinate of the beginning of the string
 *		my_y:	y coordinate of the string
 *		len:	length of the string
 *		Page:	pointer to the page that contains the text
 *	Out:	returns a pointer to a new string. If only spaces
 *		are found or len==0, NULL is returned.
 */

char *get_string(int my_x,int my_y,int len,VtPage *page,int *error) {
	char	*ptr,*ptr2;
	char	*tempstring;
	int	i,my_x_end;
	int	flag;
	int	templen;

	assert(page!=NULL);
	assert(len>0);
	assert(my_x>0);
	assert(my_x<40);
	assert(my_y>0);
	assert(my_y<25);
	
	tempstring=NULL;
	
	my_x_end=my_x+len-1;	//not shure if this is correct, but it works

	ptr=&page->page_text[my_y][my_x];
	ptr2=&page->page_text[my_y][my_x_end];

//	g_message("X:%i Y:%i\n",my_x,my_y);
//	g_message("%s\n",page->page_text[my_y]);

	flag=FALSE;
	templen=len;
	for (i=0;i<templen-1;i++) {	//find first non-space char
		if (isspace(page->page_text[my_y][my_x])!=0) {
			if (flag==FALSE) {
				my_x++;
				len--;
			}
		} else {
			flag=TRUE;
		}
	}

	flag=FALSE;
	templen=len;
	for (i=0;i<templen;i++) {	//find last non-space char
		if (isspace(page->page_text[my_y][my_x_end])!=0) {
			if (flag==FALSE) {
				my_x_end--;
				len--;
			}
		} else {
			flag=TRUE;
		}
	}

	if (len>0) {
		tempstring=malloc(len+2);
		memset(tempstring,0,len+2);
		strncpy(tempstring,&page->page_text[my_y][my_x],len);

		// If there's a '?' in the string, we have a reception error
		if (strchr(tempstring,'?')!=NULL) {
			*error=1;
		}
	}


	return tempstring;
}

/*
 *	This function extracts a double from a videotext page.
 *	It first uses get_string, then it replaces ',' by '.'
 *	and does an sscanf.
 *	In:	my_x:	x coordinate of the beginning of the string
 *		my_y:	y coordinate of the string
 *		len:	length of the string
 *		Page:	pointer to the page that contains the text
 *	Out:	returns a double if everything went well.
 *		If get_string could not find a string, -1.0 is returned.
 */

double get_double(int my_x,int my_y,int len,VtPage *page,int *error) {
	char	*temp=NULL;
	double	value;
	char	*scanner;
	int	myerror=0;
	
	temp=get_string(my_x,my_y,len,page,&myerror);	// Clean up that string

	scanner=temp;
	if (temp!=NULL) {
		while (*scanner!=0) {
			if (*scanner==',') {
				*scanner='.';
			} else {
				if (!isdigit(*scanner) && *scanner!='-' && *scanner!=' ') {
					myerror=1;
				}
			}
			scanner++;
		}

		if (myerror==0) {
			sscanf(temp,"%le",&value);
		} else {
			*error=1;
		}
		g_free(temp);

		return value;
	} else {
		return -1;
	}
}

/*
 *	This is a brutal escaping function, which is needed to generate
 *	valid table names out of weird stock names. It consequently
 *	replaces everything that gives isalnum()==0 by an underscore.
 *	In:	name:	pointer to the string to escape
 *	Out:	returns the same pointer
 */

char *escape_name(char *name) {
	int	i;

	assert(name!=NULL);

	// Replace every char that is not alnum with "_"

	if (name!=NULL) {

		for (i=0;i<strlen(name);i++) {
			if (isalnum(*(name+i))==0) {
				*(name+i)='_';
			}
		}
	}

	return name;
}

char *comma_to_dot(char *string) {
	int	i;

	assert(string!=NULL);
	
	for (i=0;i<strlen(string);i++) {
		if (string[i]==',') string[i]='.';
	}
	
	return string;
}

void add_stock_to_category(char *cat_tname, char *sname) {
	char	*stname=get_stock_tname(sname,NULL);
	char	dbquery[1000];
	MYSQL_RES *result_set;

	assert(cat_tname!=NULL);
	assert(sname!=NULL);

	sprintf(dbquery,"select StockTableName from %s where StockTableName=\'%s\'",cat_tname,stname);		
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);
	
//	assert(result_set!=NULL);
	
	if (mysql_num_rows(result_set)==0) {
		sprintf(dbquery,"insert into %s (StockTableName) values(\'%s\')",cat_tname,stname);
		mysql_query(mysql,dbquery);
	}
	
	mysql_free_result(result_set);	
}

void on_unknown_stock_tname_key_press(GtkEntry *entry) {

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tname_ok_checkbutton),FALSE);
}

/*
 * This callback automatically updates the tablename entry in the "unknown stock"
 * dialog.
 */

void on_unk_name_changed(GtkEntry *entry) {
	char	*hname;
	char	*he_name;
	int	error;

	gtk_widget_realize(GTK_WIDGET(unknown_stock_name_entry));
	hname=gtk_entry_get_text(unknown_stock_name_entry);

	assert(hname!=NULL);

	error=0;
	he_name=get_stock_tname(hname,&error);	// 1 Query per keystroke :(
	if (error==0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tname_ok_checkbutton),TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tname_ok_checkbutton),FALSE);
	}

/*	he_name=strdup(hname);
	he_name=escape_name(he_name);
*/
	if (strcmp(he_name,"")==0) {
		gtk_entry_set_text(unknown_stock_tname," ");
	} else {
		gtk_entry_set_text(unknown_stock_tname,strdup(he_name));
	}
	
}


int new_stock(char *name) {
	char	dbquery[1000];
	MYSQL_RES *result_set;
	int	status;
	char	stock_name[80];

	if (name!=NULL) {
/*		if (g_hash_table_lookup(NameCache,name)!=NULL) {
//			printf("New stock: %s\n",name);
			return TRUE;
		} else return FALSE;
*/
		mysql_escape_string(stock_name,name,strlen(name));		

		sprintf(dbquery,"select Name from _TableNames where Name like \'%s\'",stock_name);
		mysql_query(mysql,dbquery);
		result_set=mysql_store_result(mysql);

		assert(result_set!=NULL);

		if (mysql_num_rows(result_set)==0) {
			status=TRUE;
//			g_print("New stock: %s\n",name);
		} else {
			status=FALSE;
		}
		mysql_free_result(result_set);	
		return status;
		
	} else {
		return FALSE;
	}
}

void unk_set_existing(int row,char *name) {
	char	dbquery[1000];
	char	e_name[82];
	MYSQL_RES *result_set;
	MYSQL_ROW result_row;
	int	match,i;
	GdkColor color;

	gtk_clist_freeze(unknown_dialog_existing_stocks_clist);
	gtk_clist_clear(unknown_dialog_existing_stocks_clist);

	mysql_escape_string(e_name,name,strlen(name));

	sprintf(dbquery,"select Name,TName,SOUNDEX(Name),SOUNDEX(\'%s\') from _TableNames where TName not like '\\_%%' order by Name",e_name);
	mysql_query(mysql,dbquery);
	result_set=mysql_store_result(mysql);

	assert(result_set!=NULL);

	gdk_color_alloc(gtk_widget_get_colormap(GTK_WIDGET(unknown_dialog_existing_stocks_clist)),&color);
	gdk_color_parse("SkyBlue1",&color);

	match=-1;
	i=-1;
	while ((result_row=mysql_fetch_row(result_set))!=NULL) {
		i++;
		gtk_clist_append(unknown_dialog_existing_stocks_clist,result_row);
		if (match==-1 && strcmp(result_row[2],result_row[3])==0) {
			match=i;
		}
		
		if (strcmp(result_row[2],result_row[3])==0) {
			//markup matches
//			g_print("match: %s\t%s\t%i\n",name,result_row[0],i);
			gtk_clist_set_background(unknown_dialog_existing_stocks_clist,i,&color);
			// If we have a clear match, make sure it is displayed
			if (strcmp(result_row[0],name)==0) {
				match=i;
			}
		}
	}
	mysql_free_result(result_set);
	gtk_clist_thaw(unknown_dialog_existing_stocks_clist);
		
	if (match==-1) {
		gtk_clist_moveto(unknown_dialog_existing_stocks_clist,row,0,0.0,0.0);
	} else {
		gtk_clist_moveto(unknown_dialog_existing_stocks_clist,match,0,0.0,0.0);
	}
}

/*
 * This callback copies the selected Table name into the table name entry.
 */

void on_unknown_dialog_existing_stocks_clist_select_row(GtkWidget *widget,
                              gint row,
                              gint column,
                              GdkEventButton *event,
                              gpointer data) {
	char	*tname;
	
	gtk_clist_get_text(unknown_dialog_existing_stocks_clist,row,1,&tname);

	assert(tname!=NULL);

	gtk_entry_set_text(unknown_stock_tname,tname);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tname_ok_checkbutton),TRUE);

}

/*
 *	These global symbols should only be used in on_unknown_dialog_show
 */

double	_uds_value;
double	_uds_minimum;
double	_uds_maximum;
double	_uds_volume;
char	*_uds_value_c;
char	*_uds_minimum_c;
char	*_uds_maximum_c;
char	*_uds_volume_c;
char	*_uds_name;
char	*_uds_name2;
int	_uds_row;

/*
 *	These global symbols should only be used in on_unknown_dialog_close
 */
 
char	*_udc_name;
char	*_udc_name2;
char	*_udc_value_c;
char	*_udc_maximum_c;
char	*_udc_minimum_c;
char	*_udc_volume_c;

void on_usd_ok_clicked(GtkWidget *widget, gpointer user_data) {
	int	column;

	gtk_clist_get_selection_info(unknown_dialog_existing_stocks_clist,
						0,0,&_uds_row,&column);

	_udc_value_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_value))));

	_udc_maximum_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_maximum))));
	_udc_minimum_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_minimum))));
	_udc_volume_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_volume))));
	_udc_name2=gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_name)));
	_udc_name=gtk_entry_get_text(unknown_stock_tname);
}

void on_unknown_dialog_show(GtkWidget *widget, gpointer user_data) {
	assert(_uds_name2!=NULL);

	gtk_widget_realize(GTK_WIDGET(unknown_stock_name_entry));
	gtk_widget_realize(GTK_WIDGET(unknown_stock_tname));
	gtk_widget_realize(GTK_WIDGET(unknown_stock_maximum));
	gtk_widget_realize(GTK_WIDGET(unknown_stock_minimum));
	gtk_widget_realize(GTK_WIDGET(unknown_stock_volume));

//	while (g_main_iteration(FALSE));
			
//	g_print("%s\n",name2);
			
	gtk_entry_set_text(unknown_stock_name_entry,strdup(_uds_name2));
	//gtk_entry_set_text(unknown_stock_tname_entry,strdup(name));

	if (_uds_value_c!=NULL) gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_value)),_uds_value_c);
	else gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_value)),"");
				
	if (_uds_maximum==-1) {
		gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_maximum))," ");
		gtk_widget_set_sensitive(GTK_WIDGET(gnome_entry_gtk_entry(unknown_stock_maximum)),FALSE);
	} else {
		gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_maximum)),_uds_maximum_c);
		gtk_widget_set_sensitive(GTK_WIDGET(gnome_entry_gtk_entry(unknown_stock_maximum)),TRUE);
	}
	if (_uds_minimum==-1) {
		gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_minimum))," ");
		gtk_widget_set_sensitive(GTK_WIDGET(gnome_entry_gtk_entry(unknown_stock_minimum)),FALSE);
	} else {
		gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_minimum)),_uds_minimum_c);
		gtk_widget_set_sensitive(GTK_WIDGET(gnome_entry_gtk_entry(unknown_stock_minimum)),TRUE);
	}
	if (_uds_volume==-1) {
		gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_volume))," ");
		gtk_widget_set_sensitive(GTK_WIDGET(gnome_entry_gtk_entry(unknown_stock_volume)),FALSE);
	} else {
		gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_volume)),_uds_volume_c);
		gtk_widget_set_sensitive(GTK_WIDGET(gnome_entry_gtk_entry(unknown_stock_volume)),TRUE);
	}

	unk_set_existing(_uds_row,strdup(_uds_name2));

}

/*
 *	This function uses the above functions to extract all the
 *	information from the videotext page. It constructs an SQL
 *	"insert into" statement and puts the data into the database.
 *	In:	mysql:	pointer to the MYSQL object to use
 *		page:	pointer to the page to process
 */

void read_quotes_from_page(MYSQL *mysql,VtPage *page) {
	gint	num_rows;
	int	i;
	int	x,y,len;
	
	char	*name=NULL;
	char	*name2=NULL;
	char	*catname;
	char	tempstring[100];
	char	table_name[80];
	char	status_message[100];
	char	dbquery[1000];
	double	value;
	double	volume;
	double	maximum;
	double	minimum;
	int	error=0;
	int	unk_button_pressed;

	char	*value_c;
	char	*minimum_c;
	char	*maximum_c;
	char	*volume_c;

	int	entry_error;

	struct tm *tm_time;
	
/*	double	opening;
	double	closing;
*/
	assert(page!=NULL);
	
	num_rows=get_num_rows(&page->page_selections[SEL_TOPBOTTOM]);
	y=get_y_start(&page->page_selections[SEL_TOPBOTTOM]);

	for (i=0;i<num_rows;i++) {
		error=0;
		minimum_c=NULL;
		maximum_c=NULL;
		volume_c=NULL;
	
		// Get Name
		x=get_x_start(&page->page_selections[SEL_NAME]);
		len=get_x_end(&page->page_selections[SEL_NAME])-x;
		name2=get_string(x,y,len,page,&error);
		if (name2!=NULL) {
			name=get_stock_tname(name2,NULL);
//			name=strdup(name2);
//			name=escape_name(name);
			sprintf(status_message,"Parsing %s",name2);
			gnome_appbar_set_status(vtwin_appbar,status_message);
			while (g_main_iteration(FALSE));
		}
		
//		name=escape_name(get_string(x,y,len,page));

		// Get Value		
		x=get_x_start(&page->page_selections[SEL_VALUE]);
		len=get_x_end(&page->page_selections[SEL_VALUE])-x;
		value=get_double(x,y,len,page,&error);
		if (value==-1) {
			name=NULL;
			name2=NULL;
		}

		// Get Value as string
		x=get_x_start(&page->page_selections[SEL_VALUE]);
		len=get_x_end(&page->page_selections[SEL_VALUE])-x;
		value_c=get_string(x,y,len,page,&error);

		if (page->page_selections[SEL_VOLUME].disabled==FALSE) {
			// Get Volume
			x=get_x_start(&page->page_selections[SEL_VOLUME]);
			len=get_x_end(&page->page_selections[SEL_VOLUME])-x;
			volume=get_double(x,y,len,page,&error);

			// Get Volume as string
			x=get_x_start(&page->page_selections[SEL_VOLUME]);
			len=get_x_end(&page->page_selections[SEL_VOLUME])-x;
			volume_c=get_string(x,y,len,page,&error);
		} else { volume=-1; }

		if (page->page_selections[SEL_MINIMUM].disabled==FALSE) {
			// Get Minimum
			x=get_x_start(&page->page_selections[SEL_MINIMUM]);
			len=get_x_end(&page->page_selections[SEL_MINIMUM])-x;
			minimum=get_double(x,y,len,page,&error);

			// Get Minimum as string
			x=get_x_start(&page->page_selections[SEL_MINIMUM]);
			len=get_x_end(&page->page_selections[SEL_MINIMUM])-x;
			minimum_c=get_string(x,y,len,page,&error);
		} else { minimum=-1; }

		if (page->page_selections[SEL_MAXIMUM].disabled==FALSE) {
			// Get Maximum
			x=get_x_start(&page->page_selections[SEL_MAXIMUM]);
			len=get_x_end(&page->page_selections[SEL_MAXIMUM])-x;
			maximum=get_double(x,y,len,page,&error);

			// Get Maximum as string
			x=get_x_start(&page->page_selections[SEL_MAXIMUM]);
			len=get_x_end(&page->page_selections[SEL_MAXIMUM])-x;
			maximum_c=get_string(x,y,len,page,&error);
		} else { maximum=-1; }

		if ((error!=0 || new_stock(name2)) && name!=NULL) {
			// Ask the user about this stock
			
			// Show the user the page with the error
			display_this_page(page);

			entry_error=TRUE;
			while (entry_error==TRUE) {

				_udc_name=NULL;

				_uds_name=name;
				_uds_name2=name2;
				_uds_value=value;
				_uds_value_c=value_c;
				_uds_maximum=maximum;
				_uds_maximum_c=maximum_c;
				_uds_minimum=minimum;
				_uds_minimum_c=minimum_c;
				_uds_volume=volume;
				_uds_volume_c=volume_c;
			
				unk_button_pressed=gnome_dialog_run(unknown_dialog);
			
//				gtk_clist_get_selection_info(unknown_dialog_existing_stocks_clist,
//							0,0,&_uds_row,&column);
						
				gnome_dialog_close(unknown_dialog);
				while (g_main_iteration(FALSE));

				if (unk_button_pressed==-1 || unk_button_pressed==1) {
					// Ignore it
//					gnome_dialog_close(unknown_dialog);
					error=1;
					entry_error=FALSE;
				}

				if (unk_button_pressed==0) {
					// Accept the changes
					error=0;
					name=strdup(_udc_name);
					name2=strdup(_udc_name2);
//					_udc_name2=gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_name)));
//					name=gtk_entry_get_text(unknown_stock_tname);
//					value_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_value))));
					if (sscanf(_udc_value_c," %le ",&value)!=1) {
						error_dialog("You entered a bad number for the value!");
						error=1;
					}

					if (maximum!=-1) {
//						maximum_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_maximum))));
						if (sscanf(_udc_maximum_c," %le ",&maximum)!=1) {
							error_dialog("You entered a bad number for the maximum!");
							error=1;
						}
					}
					if (minimum!=-1) {
//						minimum_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_minimum))));
						if (sscanf(_udc_minimum_c," %le ",&minimum)!=1) {
							error_dialog("You entered a bad number for the minimum!");
							error=1;
						}
					}
					if (volume!=-1) {
						//Maybe filter out dots ? Like in 1.000.000?
//						volume_c=comma_to_dot(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(unknown_stock_volume))));
						if (sscanf(_udc_volume_c," %le ",&volume)!=1) {
							error_dialog("You entered a bad number for the volume!");
							error=1;
						}
					}
				if (error==0) entry_error=FALSE;
				
				}

//				gnome_dialog_close(unknown_dialog);

			}
		}

		if (error==0 && name!=NULL) {
			set_stock_names(name,name2);

			// Make sure it goes into the right category
			if (strcmp(page->category,"[none]")!=0) {
				// It's not [none]
//				g_print("%s\n",page->Category);
				if (name2!=NULL) {
					catname=get_category_tname(page->category);
					add_stock_to_category(catname,name2);
				}
			}
		
//			g_message("%s \t%.2f\n",name,value);
			mysql_escape_string(table_name,name,strlen(name));		
			sprintf(dbquery,"create table %s(value double,volume double,minimum double,maximum double,time timestamp primary key)",table_name);
//			g_message("%s\n",dbquery);

			mysql_real_query(mysql,dbquery,strlen(dbquery));

			sprintf(dbquery,"insert into %s(value",table_name);

			if (volume!=-1) {
				strcat(dbquery,",volume");
			}
			if (minimum!=-1) {
				strcat(dbquery,",minimum");
			}
			if (maximum!=-1) {
				strcat(dbquery,",maximum");
			}

			strcat(dbquery,",time) VALUES(");
			sprintf(tempstring,"%f",value);
			strcat(dbquery,tempstring);

			if (volume!=-1) {
				sprintf(tempstring,",%f",volume);
				strcat(dbquery,tempstring);
			}
			if (minimum!=-1) {
				sprintf(tempstring,",%f",minimum);
				strcat(dbquery,tempstring);
			}
			if (maximum!=-1) {
				sprintf(tempstring,",%f",maximum);
				strcat(dbquery,tempstring);
			}

			tm_time=localtime(&page->time);

			sprintf(tempstring,",\'%.4i-%.2i-%.2i %.2i:%.2i:%.2i\'",tm_time->tm_year+1900,
							   tm_time->tm_mon+1,
							   tm_time->tm_mday,
							   tm_time->tm_hour,
							   tm_time->tm_min,
							   tm_time->tm_sec);

//			free(tm_time);
			strcat(dbquery,tempstring);

			strcat(dbquery,")");

//			g_print("%s\n",dbquery);

			mysql_real_query(mysql,dbquery,strlen(dbquery));

			g_free(name);
			name=NULL;
		}

		y++;
	}
}

/*
 *	This function is called when the user clicks OK. It saves the
 *	selections he has made into the database and reads data out
 *	of any captured videotext pages, and saves it too.
 *	In/Out:	Std. Gtk+ callback.
 */

// This should probably capture the pages itself if the user hasn't done it

void on_vtwinok_clicked(GtkWidget *widget, gpointer user_data) {

	// Save Data to DB here
	
	if (mysql_query(mysql,"drop table _vtsettings")!=0) {
//		g_message("Could not drop old table. It might not have existed before.");
	}

	mysql_query(mysql,"create table _vtsettings (row int,pagenum int,subpagenum int,filename text,selection blob,Category text)");

	// Save VT Settings

	gnome_appbar_set_status(vtwin_appbar,"Updating Videotext settings");

	vtpages=&vtpage_header;
	while (vtpages->next!=NULL) {
		vtpages=vtpages->next;
		add_vt_settings_to_db(mysql,vtpages);
	}

	// Parse acquired VT Data

	row=0;

	vtpages=&vtpage_header;
	while (vtpages->next!=NULL) {
		vtpages=vtpages->next;
		read_quotes_from_page(mysql,vtpages);
	}

	gnome_appbar_refresh(vtwin_appbar);
	
	gtk_widget_hide(vt_box);
	
	update_tablelist(mysql);
}

/*
 *	This is called when the user clicks cancel.
 *	In/Out: Std. Gtk+ callback.
 */

void on_vtwincancel_clicked(GtkWidget *widget, gpointer user_data) {
	gtk_widget_hide(vt_box);
}

/*
 *	This is called when the user closes the window using the WM.
 *	In/Out: Std. Gtk+ callback.
 */

gint on_vtwin_delete_event(GtkWidget *widget, gpointer user_data) {
	gtk_widget_hide(widget);
	return TRUE;
}

/*
 *	This updates selected_row whenever the user clicks onto a new
 *	row. Couldn't find a better way.
 *	In/Out: Std. Gtk+ callback.
 */

void on_pagelist_select_row(GtkWidget *clist,gint row,gint column, GdkEventButton
				*event,gpointer data ) {
	selected_row=row;
}

/*
 *	This deletes a page from the linked list and the pagelist.
 *	If the page has subpages, they are deleted too. It is necessary
 *	to keep row_number consistent (no holes, no doubles, in order).
 *	Thus, all the pages following the deleted one have their row_number
 *	decremented by 1 (they slide up 1 row).
 *	In/Out: Std. Gtk+ callback.
 */

void on_subpage_clicked(GtkWidget *widget, gpointer user_data) {
	display_this_page(NULL);
	gtk_clist_remove(page_list,selected_row);

	vtpages=&vtpage_header;

	while (vtpages->next!=NULL) {
		vtpages=vtpages->next;
		if (vtpages->row_number==selected_row) {
			vttemp=vtpages->next;
			vtpages=vtpages->previous;
			g_free(vtpages->next);
			vtpages->next=vttemp;
			if (vttemp!=NULL) {
				vttemp->previous=vtpages;
				
			}
		}
	}
	
	// now decrement the row number of any row>selected_row by 1
	
	vtpages=&vtpage_header;
	while (vtpages->next!=NULL) {
		vtpages=vtpages->next;
		if (vtpages->row_number>selected_row) {
			vtpages->row_number-=1;
		}
	}

}

/*
 *	This shows the "Add Page" dialog when the user clicks add.
 *	In/Out: Std. Gtk+ callback.
 */

void on_addpage_clicked(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_run(add_page_dialog);
}

/*
 *	This happens when the user clicks "ok" in the "Add page" dialog.
 *	It creates the necessary structures at the end of the linked list
 *	and initializes them.
 *	In/Out: Std. Gtk+ callback.
 */
 
void on_buttonaddok_clicked(GtkWidget *widget, gpointer user_data) {
	char	page_d[4][50];	// 3 lines a 50 chars
	char	*page_data[4];
	int	i,j,k;
	char	file_name_single_format[100]="Page%i.txt";
	char	file_name_multi_format[100]="Page%i.%i.txt";

	vtpages=&vtpage_header;

	for (i=0;i<3;i++) {
		page_data[i]=page_d[i];
	}

	while (vtpages->next!=NULL) {
		vtpages=vtpages->next;
	}

//	g_message("notebook: %i\n",gtk_notebook_get_current_page(AddPageNoteBook));

	single_page_num=gtk_spin_button_get_value_as_int(single_page_num_spin);
	multi_page_num=gtk_spin_button_get_value_as_int(multi_page_num_spin);
	multi_page_num_start=gtk_spin_button_get_value_as_int(multi_page_start_spin);
	multi_page_num_end=gtk_spin_button_get_value_as_int(multi_page_end_spin);

	if (gtk_notebook_get_current_page(add_page_notebook)==0) {
		//Single Page
		
		sprintf(page_data[0],"%i",single_page_num);
		sprintf(page_data[1],"%s","-");
		sprintf(page_data[2],"%s","-");
		page_data[3]=NULL;
		gtk_clist_append(page_list,page_data);
		gnome_dialog_close(add_page_dialog);
		
		vtpages=append_vtpage(vtpages);
		init_selections(vtpages);
		
		vtpages->page_number=single_page_num;
		vtpages->sub_page_number=-1;
		vtpages->row_number=page_list->rows-1;
		sprintf(vtpages->filename,file_name_single_format,vtpages->page_number);
		
	} else {
		//Multiple Pages
		
		sprintf(page_data[0],"%i",multi_page_num);
		sprintf(page_data[1],"%i",multi_page_num_start);
		sprintf(page_data[2],"%i",multi_page_num_end);
		page_data[3]=NULL;
		
		if (multi_page_num_start<=multi_page_num_end) {
			gtk_clist_append(page_list,page_data);
			gnome_dialog_close(add_page_dialog);

			j=multi_page_num_start;
			k=multi_page_num_end;
			for (;j<=k;j++) {

				vtpages=append_vtpage(vtpages);
				init_selections(vtpages);
							
				vtpages->page_number=multi_page_num;
				vtpages->sub_page_number=j;
				vtpages->row_number=page_list->rows-1;
	
				sprintf(vtpages->filename,file_name_multi_format,vtpages->page_number,vtpages->sub_page_number);
			}
		}
	}

}

/*
 *	The user clicked cancel in the "Add Page" dialog.
 *	In/Out: Std. Gtk+ callback.
 */
 
void on_buttonaddcancel_clicked(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_close(add_page_dialog);
}

/*
 *	This adjusts the display of the GnomeCanvas. It takes the page
 *	pointed to by "displayed_page" and updates everything according
 *	to it.
 */

void update_selection() {
	double	c1,c2;
	int	i;

	if (displayed_page!=NULL) {
		c1=displayed_page->page_selections[actual_selection_type].c1;
		c2=displayed_page->page_selections[actual_selection_type].c2;

		if (actual_selection_type!=SEL_TOPBOTTOM) {
			points1[actual_selection_type]->coords[0]=c1;
			points1[actual_selection_type]->coords[1]=0;
			points1[actual_selection_type]->coords[2]=c1;
			points1[actual_selection_type]->coords[3]=325;

			points2[actual_selection_type]->coords[0]=c2;
			points2[actual_selection_type]->coords[1]=0;
			points2[actual_selection_type]->coords[2]=c2;
			points2[actual_selection_type]->coords[3]=325;
		} else {
			points1[actual_selection_type]->coords[0]=0;
			points1[actual_selection_type]->coords[1]=c1;
			points1[actual_selection_type]->coords[2]=280;
			points1[actual_selection_type]->coords[3]=c1;

			points2[actual_selection_type]->coords[0]=0;
			points2[actual_selection_type]->coords[1]=c2;
			points2[actual_selection_type]->coords[2]=280;
			points2[actual_selection_type]->coords[3]=c2;
		}

		if (displayed_page->page_selections[actual_selection_type].disabled==TRUE) {
			gnome_canvas_item_hide(lines1[actual_selection_type]);
			gnome_canvas_item_hide(lines2[actual_selection_type]);
			if (actual_selection_type!=SEL_TOPBOTTOM){
				gnome_canvas_item_hide(rect[actual_selection_type]);
			}
		} else {
			gnome_canvas_item_show(lines1[actual_selection_type]);
			gnome_canvas_item_show(lines2[actual_selection_type]);
			if (actual_selection_type!=SEL_TOPBOTTOM){
				gnome_canvas_item_show(rect[actual_selection_type]);
			}
		}
		gtk_toggle_button_set_active(disable_button,displayed_page->page_selections[actual_selection_type].disabled);

//		g_message("%f %f %f %f\n",x,y,x2,y2);
	
		gnome_canvas_item_set(lines1[actual_selection_type],
					"points",points1[actual_selection_type],
					NULL);

		gnome_canvas_item_set(lines2[actual_selection_type],
					"points",points2[actual_selection_type],
					NULL);

		if (actual_selection_type!=SEL_TOPBOTTOM) {
			gnome_canvas_item_set(rect[actual_selection_type],
					"x1",points1[actual_selection_type]->coords[0],
					"y1",points1[SEL_TOPBOTTOM]->coords[1],
					"x2",points2[actual_selection_type]->coords[2],
					"y2",points2[SEL_TOPBOTTOM]->coords[1],
					NULL);
		} else {
			//Adjust every Rectangle except the last one
			for (i=0;i<NUM_SELS-1;i++) {
				gnome_canvas_item_set(rect[i],
					"y1",points1[SEL_TOPBOTTOM]->coords[1],
					"y2",points2[SEL_TOPBOTTOM]->coords[1],
					NULL);
			}
		}

	}
}

//------------------------Selection Positioning-------------------------------------

/*
 *	These are called when the user moves the Selections. They are trivial.
 *	In/Out: Std. Gtk+ callback.
 */
void on_vt_up_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		double	*c1=&displayed_page->page_selections[actual_selection_type].c1;
		double	*c2=&displayed_page->page_selections[actual_selection_type].c2;
		
		if (*c1>Y_N_BOUND) {
			*c1-=ystep;
			*c2-=ystep;
		}
		update_selection();
	}
}
void on_vt_down_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		double	*c1=&displayed_page->page_selections[actual_selection_type].c1;
		double	*c2=&displayed_page->page_selections[actual_selection_type].c2;

		if (*c2<Y_P_BOUND) {
			*c1+=ystep;
			*c2+=ystep;
		}
		update_selection();
	}
}
void on_vt_left_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		double	*c1=&displayed_page->page_selections[actual_selection_type].c1;
		double	*c2=&displayed_page->page_selections[actual_selection_type].c2;
		
		if (*c1>X_N_BOUND) {
			*c1-=xstep;
			*c2-=xstep;
		}
		update_selection();
	}
}
void on_vt_right_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		double	*c1=&displayed_page->page_selections[actual_selection_type].c1;
		double	*c2=&displayed_page->page_selections[actual_selection_type].c2;

		if (*c2<X_P_BOUND) {
			*c1+=xstep;
			*c2+=xstep;
		}
		update_selection();
	}
}

void on_vt_taller_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		double	*c2=&displayed_page->page_selections[actual_selection_type].c2;
		if (actual_selection_type==SEL_TOPBOTTOM) {
			if (*c2<Y_P_BOUND) {
				*c2+=ystep;
			}
		} else {
			if (*c2<X_P_BOUND) {
				*c2+=xstep;
			}
		}
		update_selection();
	}
}
void on_vt_smaller_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		double	*c1=&displayed_page->page_selections[actual_selection_type].c1;
		double	*c2=&displayed_page->page_selections[actual_selection_type].c2;
		if (actual_selection_type==SEL_TOPBOTTOM) {
			if ((*c1+ystep)<*c2) {
				*c2-=ystep;
			}
		} else {
			if ((*c1+xstep)<*c2) {
				*c2-=xstep;
			}
		}
		update_selection();
	}
}

//--------------------------------Radio Button Stuff----------------------------
/*
 *	These are called when the user switches between selection types.
 *	In/Out: Std. Gtk+ callback.
 */

void on_radiobuttonname_clicked(GtkWidget *widget, gpointer user_data) {
	buttons_sens(FALSE,FALSE,TRUE,TRUE,TRUE,TRUE,FALSE);

	actual_selection_type=SEL_NAME;
	update_selection();
}
void on_radiobuttonvalue_clicked(GtkWidget *widget, gpointer user_data) {
	buttons_sens(FALSE,FALSE,TRUE,TRUE,TRUE,TRUE,FALSE);

	actual_selection_type=SEL_VALUE;
	update_selection();
}
void on_radiobuttonvolume_clicked(GtkWidget *widget, gpointer user_data) {
	buttons_sens(FALSE,FALSE,TRUE,TRUE,TRUE,TRUE,TRUE);

	actual_selection_type=SEL_VOLUME;
	update_selection();
}
void on_radiobuttonminimum_clicked(GtkWidget *widget, gpointer user_data) {
	buttons_sens(FALSE,FALSE,TRUE,TRUE,TRUE,TRUE,TRUE);

	actual_selection_type=SEL_MINIMUM;
	update_selection();
}
void on_radiobuttonmaximum_clicked(GtkWidget *widget, gpointer user_data) {
	buttons_sens(FALSE,FALSE,TRUE,TRUE,TRUE,TRUE,TRUE);

	actual_selection_type=SEL_MAXIMUM;
	update_selection();
}
void on_radiobuttontop_clicked(GtkWidget *widget, gpointer user_data) {
	buttons_sens(TRUE,TRUE,FALSE,FALSE,TRUE,TRUE,FALSE);

	actual_selection_type=SEL_TOPBOTTOM;
	update_selection();
}

/*
 *	This is called when the user en-/disables a particulare selection type
 *	In/Out: Std. Gtk+ callback.
 */
 
void on_disablebutton_toggled(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		displayed_page->page_selections[actual_selection_type].disabled=GTK_TOGGLE_BUTTON(widget)->active;
		update_selection();
	}
}

//------------------------------------------------------------------------------

/*
 *	This is called when the user clicks the "Copy to all subpages" button.
 *	It copies settings of the actual selection-type to all subpages.
 *	In/Out: Std. Gtk+ callback.
 */
 
void on_copytosubpages_clicked(GtkWidget *widget, gpointer user_data) {
	VtPage	*vtpages;
	int	pn;
	
	vtpages=&vtpage_header;
	
	if (displayed_page!=NULL) {
		pn=displayed_page->page_number;
		while (vtpages->next!=NULL) {
			vtpages=vtpages->next;
			if (vtpages->page_number==pn) {
				vtpages->page_selections[actual_selection_type].c1=displayed_page->page_selections[actual_selection_type].c1;
				vtpages->page_selections[actual_selection_type].c2=displayed_page->page_selections[actual_selection_type].c2;
				vtpages->page_selections[actual_selection_type].disabled=displayed_page->page_selections[actual_selection_type].disabled;
			}
		}
	}
}

/*
 *	This is called when the user selects "Get VT Quotes" from the menu.
 *	In/Out: Std. Gtk+ callback.
 */

void on_get_vt_quotes1_activate(GtkWidget *widget, gpointer user_data) {
	gtk_widget_show(GTK_WIDGET(vt_box));
}

/*
 *	This reads the pages that alevt-cap has saved to disk into the linked
 *	list. It knows the filename from the vtpage structure.
 *	In:	Page:	pointer to the page to read
 */

void ReadvtpageFromDisk(VtPage* page,char *homechartvtdir) {
	int	fd;
//	int	bytesread;
	int	status;
	struct stat file_stat;
	char	fn[400];

	assert(page!=NULL);
	
	sprintf(fn,"%s/%s",homechartvtdir,page->filename);

	if ((fd=open(fn,O_RDONLY))==-1) {
		// FIXME: Give a better explanation here
		perror("Couldn't load Video Page from Disk: ");
//		g_message("You may have run out of disk space.\n");
		exit(1);
	}

	stat(fn,&file_stat);
	page->time=file_stat.st_mtime;	// modification time

	memset(&page->page_text[0],0,sizeof(page->page_text));
//	bytesread=0;
//	while (bytesread<984) {
//		status=read(fd,&page->page_text[0],984-bytesread);
		status=read(fd,&page->page_text[0],984);
		if (status==-1) {
			perror("Read VT Page");
			exit(1);
		}
//		bytesread+=status;
//	}
	close(fd);
}

void on_selectcategory_entry_changed(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL) {
		sprintf(displayed_page->category,"%s",gtk_entry_get_text(GTK_ENTRY(widget)));
	}
}

/*
 *	This is called when the user clicks the button labeled "->" (Nextpage_display)
 *	In/Out: Std. Gtk+ callback.
 */

void on_nextpagedisplay_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL)
		if (displayed_page->next!=NULL) display_this_page(displayed_page->next);
}

/*
 *	This is called when the user clicks the button labeled "<-" (Previous page display)
 *	In/Out: Std. Gtk+ callback.
 */

void on_previouspagedisplay_clicked(GtkWidget *widget, gpointer user_data) {
	if (displayed_page!=NULL)
		if (displayed_page->previous!=&vtpage_header)
			if (displayed_page->previous!=NULL) display_this_page(displayed_page->previous);
}


void on_vttimeoutwindow_show(GtkWidget *widget, gpointer user_data) {
	gtk_progress_configure(vt_progress_bar,0.0,0.0,100.0);
}

gint on_vttimeoutwindow_delete_event(GtkWidget *widget, gpointer user_data) {
	kill(cpid,SIGTERM);
	child_error=2;
	gtk_widget_hide(widget);
	return TRUE;
}

void on_vttimeoutcancel_clicked(GtkWidget *widget, gpointer user_data) {
	kill(cpid,SIGTERM);
	child_error=2;
}

/*
 *	This is called when the user clicks the "Get Pages" button.
 *	It assembles a commandline for alevt-cap from the linked list of pages.
 *	Then it executes alevt-cap, waits for it to finish, and reads the
 *	pages from the disk into the linked list.
 *	In/Out: Std. Gtk+ callback.
 */

void on_getvtbutton_clicked(GtkWidget *widget, gpointer user_data) {
	int	i;
	int	childstatus;
	
	char	prog1[15]="alevt-cap";
#ifdef OLDBTTV
	char	prog11[15]="-oldbttv";
#endif
	char	prog2[15]="-name";
	char	prog3[300];
	
	char	*ale_args[2000];	// FIXME: This should be dynamic in size
	int	arg_count;
	char	ale_single_page[10]="%i";
	char	ale_multi_page[10]="%i.%i";

	char	temp_string[50];

	char	*homedir;
	char	homechartvtdir[250];

	gfloat	progress_bar_value;

	if (page_list->rows!=0) {

	// First free any linked vtpage List that might exist

	vtpages=&vtpage_header;		

	// Get the home directory & create .gchartman/videotext/
	
	homedir=strdup(getenv("HOME"));
	sprintf(homechartvtdir,"%s/.gchartman/videotext",homedir);
	sprintf(prog3,"%s/Page%%s.txt",homechartvtdir);

	mkdir(homechartvtdir,S_IRWXU);

	// create the alevt-cap arguments

/* use these defines for testing. No capturing is done when NOALEVTCAP is defined,
 * and pages are only read from disk
 */

//#define NOALEVTCAP
#undef NOALEVTCAP

#ifndef NOALEVTCAP
	arg_count=0;
	ale_args[arg_count]=g_strdup(prog1);
	arg_count++;
#ifdef OLDBTTV
	ale_args[arg_count]=g_strdup(prog11);	// -oldbttv option
	arg_count++;
#endif
	ale_args[arg_count]=g_strdup(prog2);
	arg_count++;
	ale_args[arg_count]=g_strdup(prog3);
	arg_count++;

	while (vtpages->next!=NULL) {
		vtpages=vtpages->next;
		if (vtpages->sub_page_number==-1) {
			//Single Page
			sprintf(temp_string,ale_single_page,vtpages->page_number);
		} else {
			//Multi Page
			sprintf(temp_string,ale_multi_page,vtpages->page_number,vtpages->sub_page_number);
		}
		ale_args[arg_count]=g_strdup(temp_string);
		arg_count++;
	}
	
	update_selection(vtpages);

	ale_args[arg_count]=NULL;
	arg_count++;

	child_error=0;

	if ((cpid=fork())==-1) { perror("Fork:");}
	
	if (cpid==0) {	// Child process
		g_message("Child is running...\n");
		execvp((char *)&prog1,ale_args);
		perror("\nexternal Program error: ");
		child_error=1;
		exit(1);
	}
	
	if (cpid!=0) {	// Parent process
		g_message("Waiting for alevt-cap...");

		gtk_widget_show(vt_timeout_window);
		progress_bar_value=0.0;

		while (waitpid(cpid,&childstatus,WNOHANG)==0) {
			while (g_main_iteration(FALSE));
			usleep(VT_TIMEOUT_DELAY);
			gtk_progress_set_value(vt_progress_bar,progress_bar_value);
			progress_bar_value+=VT_TIMEOUT_INC;
			if (progress_bar_value>=100.0) {
				kill(cpid,SIGTERM);
				child_error=2;
			}
		}

		
		if (WIFEXITED(childstatus)==0) {
			g_message("Timeout reached\n");
		} else {
			if ((WEXITSTATUS(childstatus)!=0)||(child_error!=0)) {
				g_message("\nThere was a problem running alevt-cap.\n");
				child_error=1;
			} else {
				g_message("done\n");
			}
		}


		for (i=0;i<arg_count;i++) {	
			g_free(ale_args[i]);
		}

		gtk_widget_hide(vt_timeout_window);

		// Read the pages from the disk here

#endif

		vtpages=&vtpage_header;
		if (child_error==0) {
			while (vtpages->next!=NULL) {
				vtpages=vtpages->next;
				ReadvtpageFromDisk(vtpages,homechartvtdir);	
			}
			// Display the first page
			display_this_page(vtpage_header.next);
		} else {
			display_this_page(NULL);
			if (child_error==1) {
				error_dialog("There was a problem with alevt-cap!");
			}
		}

		update_tablelist(mysql);

#ifndef NOALEVTCAP		
	}
#endif
	}
}
