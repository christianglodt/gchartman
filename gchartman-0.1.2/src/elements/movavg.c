#include "../module.h"

#define	MOVAVGGLADE_FILE PLUGINDIR"/movavgprop.glade"

struct movavg_data {
	Quote	*quote_vector;
	int	quote_count;
	
	guint8	r,g,b,a;
	gfloat	width;
	int	days;
	double	pix_per_eur;
};

static int	movavg_index;
static time_t	movavg_lasttime;

static GnomeCanvasPoints *MovAvg_Points;

static GnomeColorPicker *movavg_cpick;
static GtkSpinButton *movavg_widthspin;
static GtkSpinButton *movavg_daysspin;

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=FALSE;
	caps->deleteable=TRUE;
	caps->in_toolbar=TRUE;
	caps->new_area=FALSE;
	caps->parent=TRUE;
	caps->child=TRUE;
	caps->name=strdup("Moving Average");
	caps->type=strdup("gchartman/moving_average");
	caps->icon=strdup("movavg.png");
	return caps;
}
/*
char *get_instance_name(void *_data) {
	char	name[30];
	struct movavg_data *data=(struct movavg_data *)_data;
	
	sprintf(name,"Mov. Avg. (%i)",data->days);
	return strdup(name);
}
*/
void free_object(ChartInstance *ci) {
	struct movavg_data *data=(struct movavg_data *)ci->instance_data;

	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));
	g_free(data);
}
/*
static gint movavg_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer data) {

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
static int	movavg_last_mday;

static void movavg_set_points(GnomeCanvasPoints *points,Quote *quote_vector,int *quote_count,Quote *new_quote_vector,int *new_quote_count,int days,int do_scale,struct movavg_data *data) {
	int	i,j;
	double	sum;
	double	temp[*quote_count-days];
	int	tempindex=0;

	// First Coord is newest

	movavg_index=0;

	for (i=1;i<((*quote_count)-days)+1;i++) {

		points->coords[movavg_index]=CHARTER_X(i-1);

		sum=0;
		// Sum up all values for the last  days
		for (j=0;j<days;j++) {
			sum+=quote_vector[i+j].value;
		}

		if (do_scale) {
			points->coords[movavg_index+1]=CHARTER_Y(sum/days);
			temp[tempindex]=-CHARTER_RY(sum/days);
		} else {
			points->coords[movavg_index+1]=-1.0*data->pix_per_eur*(sum/days);
//			temp[tempindex]=(sum/days)/data->pix_per_eur;
			temp[tempindex]=(sum/days);
		}

		tempindex++;

		movavg_index+=2;
	}
	
	for (i=0;i<(*quote_count-days);i++) {
		new_quote_vector[i].value=temp[i];
	}

	*new_quote_count-=days;

}

static void create_movavg(ChartInstance *ci) {
	struct movavg_data *data=(struct movavg_data *)ci->instance_data;
	char	color_string[8];
	int	do_scale;

	if (ci->quote_count>1 && ci->quote_count>(data->days+1)) {
		sprintf(color_string,"#%.2X%.2X%.2X",data->r,data->g,data->b);

		movavg_lasttime=ci->quote_vector[1].timestamp;

		ci->move_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
				GNOME_TYPE_CANVAS_GROUP,
				"x",0.0,
				"y",0.0,
				NULL);

		MovAvg_Points=gnome_canvas_points_new((ci->quote_count - (data->days)));

		movavg_last_mday=-1;

		do_scale=TRUE;
		if (ci->parent) {
			// MovAvg does not scale for RSI, Momentum & Co.
			if (ci->parent->type->caps->new_area) do_scale=FALSE;
		}

		movavg_set_points(MovAvg_Points,ci->quote_vector,&ci->quote_count,ci->new_quote_vector,&ci->new_quote_count,data->days,do_scale,data);

		gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",MovAvg_Points,
				"width_units",data->width,
                                "fill_color", color_string,
				NULL);	

//		gtk_signal_connect(GTK_OBJECT(data->item),"event",(GtkSignalFunc)movavg_callback,NULL);

		gnome_canvas_points_free(MovAvg_Points);

	}
}

static void ma_update_xml_node(ChartInstance *ci) {
	struct movavg_data *data=(struct movavg_data *)ci->instance_data;
	char	temp_string[20];

	sprintf(temp_string,"%i",data->r);
	xmlSetProp(ci->xml_node,"r",temp_string);
	sprintf(temp_string,"%i",data->g);
	xmlSetProp(ci->xml_node,"g",temp_string);
	sprintf(temp_string,"%i",data->b);
	xmlSetProp(ci->xml_node,"b",temp_string);
	sprintf(temp_string,"%i",data->a);
	xmlSetProp(ci->xml_node,"a",temp_string);
	sprintf(temp_string,"%f",data->width);
	xmlSetProp(ci->xml_node,"width",temp_string);
	sprintf(temp_string,"%i",data->days);
	xmlSetProp(ci->xml_node,"days",temp_string);
}

void create(ChartInstance *ci,xmlNodePtr config_node) {
	char	name[30];
	struct movavg_data *data;

	data=(struct movavg_data *)malloc(sizeof(struct movavg_data));
	ci->instance_data=data;

	if (config_node!=NULL) {
		data->r=atoi(xmlGetProp(config_node,"r"));
		data->g=atoi(xmlGetProp(config_node,"g"));
		data->b=atoi(xmlGetProp(config_node,"b"));
		data->a=atoi(xmlGetProp(config_node,"a"));
		data->width=atof(xmlGetProp(config_node,"width"));
		data->days=atoi(xmlGetProp(config_node,"days"));

/*		Data->r=pdata->r;
		Data->g=pdata->g;
		Data->b=pdata->b;
		Data->a=pdata->a;
		Data->width=pdata->width;
		Data->days=pdata->days;
*/	} else {
		data->r=255;
		data->g=0;
		data->b=0;
		data->a=0;
		data->width=0.5;
		data->days=30;
	}	

	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/moving_average");
	ma_update_xml_node(ci);

	sprintf(name,"Mov. Avg. (%i)",data->days);
	ci->instance_name=strdup(name);
	data->pix_per_eur=pix_per_eur;

	create_movavg(ci);
}

static ChartInstance *ma_prop_ci;

void on_movavg_propertybox_apply (GnomePropertyBox *property_box, gint page_num) {
	char	name[30];
	struct movavg_data *data=(struct movavg_data *)ma_prop_ci->instance_data;

	if (page_num != -1) return;

	gnome_color_picker_get_i8(movavg_cpick,&data->r,&data->g,&data->b,&data->a);
	data->width=gtk_spin_button_get_value_as_float(movavg_widthspin);
	data->days=gtk_spin_button_get_value_as_int(movavg_daysspin);

	gtk_object_destroy(GTK_OBJECT(ma_prop_ci->move_group));

	create_movavg(ma_prop_ci);
	ma_update_xml_node(ma_prop_ci);
	sprintf(name,"Mov. Avg. (%i)",data->days);
	ma_prop_ci->instance_name=strdup(name);
	ma_prop_ci->y_offset=0.0;

//	g_print("Font: %s\n",data->fontname);	
//	g_print("Colors: %i %i %i %i\n",data->r,data->g,data->b,data->a);
}

void set_properties(ChartInstance *ci) {
	struct movavg_data *data=(struct movavg_data *)ci->instance_data;
	GnomePropertyBox *movavg_pbox;
	GladeXML *xxml;

	xxml=glade_xml_new(MOVAVGGLADE_FILE,"MovAvgPropertyBox");
      	glade_xml_signal_autoconnect(xxml);

	movavg_pbox=(GnomePropertyBox *)glade_xml_get_widget(xxml,"MovAvgPropertyBox");
	movavg_cpick=(GnomeColorPicker *)glade_xml_get_widget(xxml,"MovAvgColor");
	movavg_widthspin=(GtkSpinButton *)glade_xml_get_widget(xxml,"MovAvgWidth");
	movavg_daysspin=(GtkSpinButton *)glade_xml_get_widget(xxml,"MovAvgDays");

	//Hack away the "apply"  button
	gtk_widget_hide(GNOME_PROPERTY_BOX(movavg_pbox)->apply_button);

	gnome_property_box_changed(movavg_pbox);	//Necessary, or the callback won't be called

	gnome_color_picker_set_i8(movavg_cpick,data->r,data->g,data->b,data->a);
	gtk_spin_button_set_value(movavg_widthspin,data->width);
	gtk_spin_button_set_value(movavg_daysspin,data->days);
		
	ma_prop_ci=ci;
	gnome_dialog_run(GNOME_DIALOG(movavg_pbox));
	ma_prop_ci=NULL;
	
//	gtk_widget_destroy(pbox);
//	gtk_widget_destroy(xxml);
}

void update(ChartInstance *ci) {

	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));

	create_movavg(ci);
}

void redraw(ChartInstance *ci) {
	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));

	create_movavg(ci);
}
