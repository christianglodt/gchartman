#include "../module.h"

#define	BUYSELLGLADE_FILE PLUGINDIR"/buysellprop.glade"

struct buysell_data {
	GnomeCanvasItem	*arrow;
	GnomeCanvasItem	*text;
	GnomeCanvasItem	*text2;
	
	guint8	r,g,b,a;
	gfloat	price;
	gfloat	amount;
	int	buy;
	int	day;
	time_t	time;
	char	i_name[40];
};

/*struct buysell_persistent_data {
	guint8	r,g,b,a;
	float	price;
	float	amount;
	int	day,month,year;
	time_t	time;
	int	buy;	// TRUE == buy, FALSE == sell
};
*/
static GnomeCanvasPoints *bs_points;

static GnomeColorPicker *bs_cpick;
static GtkSpinButton *bs_pricespin;
static GtkSpinButton *bs_numstocks;
static GnomeDateEdit *bs_date;
static GtkWidget *bs_buybotton;
static GtkWidget *bs_sellbutton;

static ChartInstance *bs_global_ci;	//Only for use in PropertyBox handlers !!!

static char	*buysell_name="Buy/Sell";
static char	*buysell_type="gchartman/buysell";
static char	*buysell_icon="buysell.png";

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=FALSE;
	caps->deleteable=TRUE;
	caps->in_toolbar=TRUE;
	caps->new_area=FALSE;
	caps->parent=FALSE;
	caps->child=FALSE;
	caps->name=buysell_name;
	caps->type=buysell_type;
	caps->icon=buysell_icon;
	return caps;
}

static void bs_update_xml_node(ChartInstance *ci) {
	struct buysell_data *data=(struct buysell_data *)ci->instance_data;
	char	temp_string[20];

	sprintf(temp_string,"%i",data->r);
	xmlSetProp(ci->xml_node,"r",temp_string);
	sprintf(temp_string,"%i",data->g);
	xmlSetProp(ci->xml_node,"g",temp_string);
	sprintf(temp_string,"%i",data->b);
	xmlSetProp(ci->xml_node,"b",temp_string);
	sprintf(temp_string,"%i",data->a);
	xmlSetProp(ci->xml_node,"a",temp_string);
	sprintf(temp_string,"%f",data->price);
	xmlSetProp(ci->xml_node,"price",temp_string);
	sprintf(temp_string,"%f",data->amount);
	xmlSetProp(ci->xml_node,"amount",temp_string);
	sprintf(temp_string,"%li",data->time);
	xmlSetProp(ci->xml_node,"time",temp_string);
	sprintf(temp_string,"%i",data->buy);
	xmlSetProp(ci->xml_node,"buy",temp_string);
	sprintf(temp_string,"%i",ci->quote_vector[data->day].day);
	xmlSetProp(ci->xml_node,"day",temp_string);
	sprintf(temp_string,"%i",ci->quote_vector[data->day].month);
	xmlSetProp(ci->xml_node,"month",temp_string);
	sprintf(temp_string,"%i",ci->quote_vector[data->day].year);
	xmlSetProp(ci->xml_node,"year",temp_string);
}

void free_object(ChartInstance *ci) {
	struct buysell_data *d=(struct buysell_data *)ci->instance_data;

	if (d->arrow!=NULL) gtk_object_destroy(GTK_OBJECT(d->arrow));
	if (d->text!=NULL) gtk_object_destroy(GTK_OBJECT(d->text));
	if (d->text2!=NULL) gtk_object_destroy(GTK_OBJECT(d->text2));
	if (ci->fixed_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
	g_free(d);
}

static void create_buysell(ChartInstance *ci) {
	char	color_string[8];
	struct tm *tm_time;
	int	i;
	char	text_string[20];
	double	text_offset;

	struct buysell_data *data=(struct buysell_data *)ci->instance_data;
	ci->fixed_group=NULL;
	ci->move_group=NULL;
	data->arrow=NULL;

	if (ci->quote_count>1) {
		tm_time=localtime(&data->time);

		data->day=-1;
		for (i=1;i<ci->quote_count+1;i++) {
			if ( (ci->quote_vector[i].year==tm_time->tm_year) &&
			     (ci->quote_vector[i].month==tm_time->tm_mon) &&
			     (ci->quote_vector[i].day==tm_time->tm_mday)) {
			
				data->day=i-1;
				break;
			}
		}

		if (data->day==-1) {
			data->time=ci->quote_vector[1].timestamp;
			data->day=0;
		}

		sprintf(color_string,"#%.2X%.2X%.2X",data->r,data->g,data->b);

		ci->fixed_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
				GNOME_TYPE_CANVAS_GROUP,
				"x",0.0,
				"y",0.0,
				NULL);

		bs_points=gnome_canvas_points_new(2);

		bs_points->coords[0]=CHARTER_X(data->day);
		bs_points->coords[1]=CHARTER_Y(data->price);
		bs_points->coords[2]=CHARTER_X(data->day);
		bs_points->coords[3]=CHARTER_Y(data->price);

		if (data->buy) {
			bs_points->coords[3]+=10;
			text_offset=10;
		} else {
			bs_points->coords[3]-=10;
			text_offset=-30;
		}

		data->arrow=gnome_canvas_item_new(ci->fixed_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",bs_points,
				"width_units",1.0,
                                "fill_color", color_string,
				"first_arrowhead",TRUE,
				"last_arrowhead",FALSE,
				"arrow_shape_a",5.0,
				"arrow_shape_b",5.0,
				"arrow_shape_c",2.5,
				NULL);	

		sprintf(text_string,"%.2f",data->amount);
		data->text=gnome_canvas_item_new (ci->fixed_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", CHARTER_X(data->day),
                               "y", CHARTER_Y(data->price)+text_offset,
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_N,
                               "fill_color", "black",
                               NULL);

		sprintf(text_string,"%.2f",data->price);
		data->text2=gnome_canvas_item_new (ci->fixed_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", CHARTER_X(data->day),
                               "y", CHARTER_Y(data->price)+text_offset+10,
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_N,
                               "fill_color", "black",
                               NULL);
		
		gnome_canvas_points_free(bs_points);

	}
}

void create(ChartInstance *ci,xmlNodePtr config_node) {
	struct buysell_data *data;

	data=(struct buysell_data *)malloc(sizeof(struct buysell_data));
	ci->instance_data=data;

	if (config_node!=NULL) {
		data->r=atoi(xmlGetProp(config_node,"r"));
		data->g=atoi(xmlGetProp(config_node,"g"));
		data->b=atoi(xmlGetProp(config_node,"b"));
		data->a=atoi(xmlGetProp(config_node,"a"));
		data->price=atof(xmlGetProp(config_node,"price"));
		data->amount=atof(xmlGetProp(config_node,"amount"));
		data->day=atoi(xmlGetProp(config_node,"day"));
		data->time=atol(xmlGetProp(config_node,"time"));
		data->buy=atoi(xmlGetProp(config_node,"buy"));

	} else {
		data->buy=TRUE;
		data->r=255;
		data->g=0;
		data->b=0;
		data->a=0;
		data->price=ci->quote_vector[1].value;
		data->amount=100.0;
		data->day=1;
		data->time=ci->quote_vector[1].timestamp;
	}	

	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type",buysell_type);
	bs_update_xml_node(ci);
	create_buysell(ci);

	if (data->buy) sprintf(data->i_name,"Buy (%.2f @ %.2f)",data->amount,data->price);
	else sprintf(data->i_name,"Sell (%.2f @ %.2f)",data->amount,data->price);
	ci->instance_name=data->i_name;
}

void on_buysell_propertybox_apply (GnomePropertyBox *property_box, gint page_num) {
	struct buysell_data *data=(struct buysell_data *)bs_global_ci->instance_data;

	if (page_num != -1) return;

	gnome_color_picker_get_i8(bs_cpick,&data->r,&data->g,&data->b,&data->a);
	data->price=gtk_spin_button_get_value_as_float(bs_pricespin);
	data->amount=gtk_spin_button_get_value_as_float(bs_numstocks);
	data->time=gnome_date_edit_get_date(bs_date);
	if (GTK_TOGGLE_BUTTON(bs_buybotton)->active) {
		data->buy=TRUE;
	} else {
		data->buy=FALSE;
	}

	gtk_object_destroy(GTK_OBJECT(bs_global_ci->fixed_group));

	bs_update_xml_node(bs_global_ci);
	create_buysell(bs_global_ci);
	if (data->buy) sprintf(data->i_name,"Buy (%.2f @ %.2f)",data->amount,data->price);
	else sprintf(data->i_name,"Sell (%.2f @ %.2f)",data->amount,data->price);
	bs_global_ci->instance_name=data->i_name;
}

void set_properties(ChartInstance *ci) {
	struct buysell_data *data=(struct buysell_data *)ci->instance_data;
	GnomePropertyBox *pbox;
	GladeXML *xxml;
	
	xxml=glade_xml_new(BUYSELLGLADE_FILE,"BuySellPropertyBox");
      	glade_xml_signal_autoconnect(xxml);

	pbox=(GnomePropertyBox *)glade_xml_get_widget(xxml,"BuySellPropertyBox");
	bs_cpick=(GnomeColorPicker *)glade_xml_get_widget(xxml,"BuySellColor");
	bs_pricespin=(GtkSpinButton *)glade_xml_get_widget(xxml,"BuySellPrice");
	bs_numstocks=(GtkSpinButton *)glade_xml_get_widget(xxml,"NumStocks");
	bs_date=(GnomeDateEdit *)glade_xml_get_widget(xxml,"Date");
	bs_buybotton=(GtkWidget *)glade_xml_get_widget(xxml,"BuyButton");
	bs_sellbutton=(GtkWidget *)glade_xml_get_widget(xxml,"SellButton");

	//Hack away the "apply"  button
	gtk_widget_hide(GNOME_PROPERTY_BOX(pbox)->apply_button);

	gnome_property_box_changed(pbox);	//Necessary, or the callback won't be called

	gnome_color_picker_set_i8(bs_cpick,data->r,data->g,data->b,data->a);
	gtk_spin_button_set_value(bs_pricespin,data->price);
	gtk_spin_button_set_value(bs_numstocks,data->amount);
	gnome_date_edit_set_time(bs_date,data->time);

	if (data->buy) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bs_buybotton),TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bs_sellbutton),TRUE);

	bs_global_ci=ci;		
	gnome_dialog_run(GNOME_DIALOG(pbox));
}

void update(ChartInstance *ci) {
	struct buysell_data *data=(struct buysell_data *)ci->instance_data;
	if (ci->quote_count>1) {
		gtk_object_destroy(GTK_OBJECT(data->arrow));
		gtk_object_destroy(GTK_OBJECT(data->text));
		gtk_object_destroy(GTK_OBJECT(data->text2));
		gtk_object_destroy(GTK_OBJECT(ci->fixed_group));

		create_buysell(ci);
	}
}

void redraw(ChartInstance *ci) {
	struct buysell_data *data=(struct buysell_data *)ci->instance_data;
	if (ci->quote_count>1) {
		gtk_object_destroy(GTK_OBJECT(data->arrow));
		gtk_object_destroy(GTK_OBJECT(data->text));
		gtk_object_destroy(GTK_OBJECT(data->text2));
		gtk_object_destroy(GTK_OBJECT(ci->fixed_group));

		create_buysell(ci);
	}
}
