#include "../module.h"

#define	RSIGLADE_FILE PLUGINDIR"/rsiprop.glade"

struct rsi_data {
	GnomeCanvasItem	*line;
	GnomeCanvasItem	*rect;
	GnomeCanvasItem	*rect2;
	GnomeCanvasItem	*text;
	GnomeCanvasItem	*text2;
	GnomeCanvasItem	*text3;
	
	guint8	r,g,b,a;
	gfloat	width;
	int	days;
	int	zone;
};

/*struct rsi_pdata {
	guint8	r,g,b,a;
	float	width;
	int	days;
	int	zone;
};*/

static int	modify_qv;
static int	rsi_index;
static time_t	rsi_lasttime;

static GnomeCanvasPoints *rsi_points;

static GnomeColorPicker *rsi_cpick;
static GtkSpinButton *rsi_widthspin;
static GtkSpinButton *rsi_daysspin;
static GtkSpinButton *rsi_zonespin;

static ChartInstance *_global_ci;	//Only for use in PropertyBox handlers !!!

static char *rsi_name="RSI";
static char *rsi_type="gchartman/rsi";
static char *rsi_icon="rsi.png";

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=FALSE;
	caps->deleteable=TRUE;
	caps->in_toolbar=TRUE;
	caps->new_area=TRUE;
	caps->parent=TRUE;
	caps->child=TRUE;
	caps->name=rsi_name;
	caps->type=rsi_type;
	caps->icon=rsi_icon;
	return caps;
}

/*char *get_instance_name(void *_data) {
	char	name[30];
	struct rsi_data *data=(struct rsi_data *)_data;
	
	sprintf(name,"RSI (%i)",data->days);
	return strdup(name);
}*/

/*GnomeCanvasGroup *get_group(void *data,int size_resp) {
	struct rsi_data *d=(struct rsi_data *)data;	// Cast is necessary...
	
	return d->group;
}*/

void free_object(ChartInstance *ci) {
	struct rsi_data *d=(struct rsi_data *)ci->instance_data;

	if (d->line!=NULL) gtk_object_destroy(GTK_OBJECT(d->line));
	if (d->rect!=NULL) gtk_object_destroy(GTK_OBJECT(d->rect));
	if (d->rect2!=NULL) gtk_object_destroy(GTK_OBJECT(d->rect2));
	if (d->text!=NULL) gtk_object_destroy(GTK_OBJECT(d->text));
	if (d->text2!=NULL) gtk_object_destroy(GTK_OBJECT(d->text2));
	if (d->text3!=NULL) gtk_object_destroy(GTK_OBJECT(d->text3));
	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));
	g_free(d);
}

void update_xml_node(ChartInstance *ci) {
	struct rsi_data *data=(struct rsi_data *)ci->instance_data;
	char	temp_string[20];

	sprintf(temp_string,"%i",data->r);
	xmlSetProp(ci->xml_node,"r",temp_string);
	sprintf(temp_string,"%i",data->g);
	xmlSetProp(ci->xml_node,"g",temp_string);
	sprintf(temp_string,"%i",data->b);
	xmlSetProp(ci->xml_node,"b",temp_string);
	sprintf(temp_string,"%i",data->a);
	xmlSetProp(ci->xml_node,"a",temp_string);
	sprintf(temp_string,"%g",data->width);
	xmlSetProp(ci->xml_node,"width",temp_string);
	sprintf(temp_string,"%i",data->days);
	xmlSetProp(ci->xml_node,"days",temp_string);
	sprintf(temp_string,"%i",data->zone);
	xmlSetProp(ci->xml_node,"zone",temp_string);
}

/*
static gint rsi_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer data) {

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
			break;
		
		case GDK_LEAVE_NOTIFY:
			break;
		
		case GDK_BUTTON_PRESS:
			switch(event->button.button) {
				case 1:		// Left Mousebutton
					break;
				case 2:		// Middle Mousebutton
					break;
				case 3:		// Right Mousebutton
					break;
				default:	// Unkown Mousebutton
					break;
			}
			break;
		
		case GDK_BUTTON_RELEASE:
			break;
		
		case GDK_MOTION_NOTIFY:
			break;
		default:
			break;
		
	}
	return FALSE;
}
*/
static int	rsi_last_mday;

void rsi_set_points(GnomeCanvasPoints *points,Quote *quote_vector,int quote_count,int days,Quote *new_quote_vector, int *new_quote_count) {
	int	i,j;
	double	sum;
	double	U,D,u,d,RS,RSI;
	int	p_days,n_days;
	double	temp[quote_count-days];
	int	tempindex=0;

	// First Coord is newest

	rsi_index=0;


	for (i=1;i<(quote_count-days)+1;i++) {

		p_days=0;
		n_days=0;
		U=0;
		D=0;

		points->coords[rsi_index]=CHARTER_X(i-1);

		sum=0;
		// Sum up all values for the last  days
		for (j=0;j<days-1;j++) {
			if (quote_vector[i+j].value > quote_vector[i+j+1].value) {
				u=quote_vector[i+j].value-quote_vector[i+j+1].value;
				d=0;
				p_days++;
			} else {
				d=quote_vector[i+j+1].value-quote_vector[i+j].value;
				u=0;
				n_days++;
			}
			
			U+=u;
			D+=d;
			
			sum+=quote_vector[i+j].value;
		}
		
		U/=days;
		D/=days;
		
		RS=U/D;

		RSI=100-(100/(1+RS));
		
		points->coords[rsi_index+1]=-RSI;
		temp[tempindex]=-CHARTER_RY(RSI);

		rsi_index+=2;
		tempindex++;
	}

	if (modify_qv) {
		for (i=0;i<(quote_count-days);i++) {
			new_quote_vector[i].value=temp[i];
		}

		*new_quote_count=quote_count-days;
	}
}

void create_rsi(ChartInstance *ci) {
	char	color_string[8];
	char	text_string[10];

	struct rsi_data *data=(struct rsi_data *)ci->instance_data;

	ci->move_group=NULL;

	if ((ci->quote_count)>1 && (ci->quote_count)>(data->days+1)) {
		sprintf(color_string,"#%.2X%.2X%.2X",data->r,data->g,data->b);

		rsi_lasttime=ci->quote_vector[1].timestamp;

		ci->move_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
				GNOME_TYPE_CANVAS_GROUP,
				"x",0.0,
				"y",0.0,
				NULL);

		rsi_points=gnome_canvas_points_new(((ci->quote_count) - (data->days)));

		rsi_last_mday=-1;

		rsi_set_points(rsi_points,ci->quote_vector,ci->quote_count,data->days,ci->new_quote_vector,&ci->new_quote_count);

		data->line=gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",rsi_points,
				"width_units",data->width,
                                "fill_color", color_string,
				NULL);	

		gnome_canvas_points_free(rsi_points);

		data->rect=gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_RECT,
				"x1",1.0*CHARTER_X((ci->quote_count)-1),
				"y1",-100.0,
				"x2",0.0,
				"y2",0.0,
				"fill_color",NULL,
				"outline_color","black",
				"width_pixels",1,
				NULL);	

		data->rect2=gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_RECT,
				"x1",1.0*CHARTER_X((ci->quote_count)-1),
				"y1",-100.0*(1-((data->zone)/100.0)),
				"x2",0.0,
				"y2",-100.0*((data->zone)/100.0),
				"fill_color",NULL,
				"outline_color","black",
				"width_pixels",1,
				NULL);	

		sprintf(text_string,"%i",100-data->zone);
		data->text=gnome_canvas_item_new (ci->move_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", 5.0+1.0,
                               "y", -100.0*(1-((data->zone)/100.0)),
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_W,
                               "fill_color", "black",
                               NULL);

		sprintf(text_string,"%i",data->zone);
		data->text2=gnome_canvas_item_new (ci->move_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", 5.0+1.0,
                               "y", -100.0*((data->zone)/100.0),
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_W,
                               "fill_color", "black",
                               NULL);

		sprintf(text_string,"RSI(%i)",data->days);
		data->text3=gnome_canvas_item_new (ci->move_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", -1.0,
                               "y", -100.0,
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_NE,
                               "fill_color", "black",
                               NULL);
				
//		printf("%f,%f\n",-100.0*(1-((data->zone)/100.0)),-100.0*((data->zone)/100.0));
				
	}
}

void create(ChartInstance *ci,xmlNodePtr config_node) {
//	struct rsi_pdata *pdata=(struct rsi_pdata *)_pdata;
	struct rsi_data *data;
	char	name[30];

	data=(struct rsi_data *)malloc(sizeof(struct rsi_data));
	ci->instance_data=data;

	if (config_node!=NULL) {
		data->r=atoi(xmlGetProp(config_node,"r"));
		data->g=atoi(xmlGetProp(config_node,"g"));
		data->b=atoi(xmlGetProp(config_node,"b"));
		data->a=atoi(xmlGetProp(config_node,"a"));
		data->width=atof(xmlGetProp(config_node,"width"));
		data->days=atoi(xmlGetProp(config_node,"days"));
		data->zone=atoi(xmlGetProp(config_node,"zone"));

	} else {
		data->r=5;
		data->g=125;
		data->b=0;
		data->a=0;
		data->width=1.5;
		data->days=14;
		data->zone=30;
	}	

	modify_qv=TRUE;
	
	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/rsi");
	sprintf(name,"RSI (%i)",data->days);
	ci->instance_name=strdup(name);
	
	update_xml_node(ci);	
	create_rsi(ci);

}

/*void *get_pdata(void *_data,int *len) {
	struct rsi_data *data=(struct rsi_data *)_data;

	struct rsi_pdata *pdata=(struct rsi_pdata *)malloc(sizeof(struct rsi_pdata));
	
	pdata->r=data->r;
	pdata->g=data->g;
	pdata->b=data->b;
	pdata->a=data->a;
	
	pdata->width=data->width;
	
	pdata->days=data->days;

	pdata->zone=data->zone;
	
	*len=sizeof(*pdata);
	
	return pdata;
}*/

void on_rsi_propertybox_apply (GnomePropertyBox *property_box, gint page_num) {
	struct rsi_data *data=(struct rsi_data *)_global_ci->instance_data;
	char	name[30];

	if (page_num != -1) return;

	gnome_color_picker_get_i8(rsi_cpick,&data->r,&data->g,&data->b,&data->a);
	data->width=gtk_spin_button_get_value_as_float(rsi_widthspin);
	data->days=gtk_spin_button_get_value_as_int(rsi_daysspin);
	data->zone=gtk_spin_button_get_value_as_int(rsi_zonespin);

	gtk_object_destroy(GTK_OBJECT(_global_ci->move_group));

	modify_qv=TRUE;
	create_rsi(_global_ci);
	sprintf(name,"RSI (%i)",data->days);
	_global_ci->instance_name=strdup(name);
	
	update_xml_node(_global_ci);	
	_global_ci->y_offset=0.0;
}

void set_properties(ChartInstance *ci) {
	struct rsi_data *data=(struct rsi_data *)ci->instance_data;
	GnomePropertyBox *rsi_pbox;
	GladeXML *xxml;
	
	xxml=glade_xml_new(RSIGLADE_FILE,"rsiPropertyBox");
      	glade_xml_signal_autoconnect(xxml);

	rsi_pbox=(GnomePropertyBox *)glade_xml_get_widget(xxml,"rsiPropertyBox");
	rsi_cpick=(GnomeColorPicker *)glade_xml_get_widget(xxml,"rsiColor");
	rsi_widthspin=(GtkSpinButton *)glade_xml_get_widget(xxml,"rsiWidth");
	rsi_daysspin=(GtkSpinButton *)glade_xml_get_widget(xxml,"rsiDays");
	rsi_zonespin=(GtkSpinButton *)glade_xml_get_widget(xxml,"rsiZone");

	//Hack away the "apply"  button
	gtk_widget_hide(GNOME_PROPERTY_BOX(rsi_pbox)->apply_button);

	gnome_property_box_changed(rsi_pbox);	//Necessary, or the callback won't be called

	gnome_color_picker_set_i8(rsi_cpick,data->r,data->g,data->b,data->a);
	gtk_spin_button_set_value(rsi_widthspin,data->width);
	gtk_spin_button_set_value(rsi_daysspin,data->days);
	gtk_spin_button_set_value(rsi_zonespin,data->zone);
	_global_ci=ci;
	gnome_dialog_run(GNOME_DIALOG(rsi_pbox));
	_global_ci=NULL;
}

void update(ChartInstance *ci) {
	struct rsi_data *data=(struct rsi_data *)ci->instance_data;
	if (data->line!=NULL) gtk_object_destroy(GTK_OBJECT(data->line));
	if (data->rect!=NULL) gtk_object_destroy(GTK_OBJECT(data->rect));
	if (data->rect2!=NULL) gtk_object_destroy(GTK_OBJECT(data->rect2));
	if (data->text!=NULL) gtk_object_destroy(GTK_OBJECT(data->text));
	if (data->text2!=NULL) gtk_object_destroy(GTK_OBJECT(data->text2));
	if (data->text3!=NULL) gtk_object_destroy(GTK_OBJECT(data->text3));
	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));

	modify_qv=FALSE;
	create_rsi(ci);
}

void redraw(ChartInstance *ci) {
	struct rsi_data *data=(struct rsi_data *)ci->instance_data;
	if (data->line!=NULL) gtk_object_destroy(GTK_OBJECT(data->line));
	if (data->rect!=NULL) gtk_object_destroy(GTK_OBJECT(data->rect));
	if (data->rect2!=NULL) gtk_object_destroy(GTK_OBJECT(data->rect2));
	if (data->text!=NULL) gtk_object_destroy(GTK_OBJECT(data->text));
	if (data->text2!=NULL) gtk_object_destroy(GTK_OBJECT(data->text2));
	if (data->text3!=NULL) gtk_object_destroy(GTK_OBJECT(data->text3));
	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));

	modify_qv=FALSE;
	create_rsi(ci);
}
