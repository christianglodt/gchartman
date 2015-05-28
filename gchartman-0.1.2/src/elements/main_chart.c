#include "../module.h"

#define	MAINCHARTGLADE_FILE PLUGINDIR"/main_chartprop.glade"

struct mainchart_data {
	GnomeCanvasItem	*item;
	
	guint8	r,g,b,a;
	gfloat	width;
};

static int	mc_index;
static time_t	mc_lasttime;

static GnomeCanvasPoints *mc_points;

static GnomeColorPicker *mc_cpick;
static GtkSpinButton *mc_widthspin;

static ChartInstance *mc_global_ci;

ChartCap *get_caps() {
        ChartCap *caps;
        
        caps=(ChartCap *)g_malloc(sizeof(ChartCap));
        caps->default_create=TRUE;
        caps->deleteable=FALSE;
        caps->in_toolbar=FALSE;
        caps->new_area=FALSE;
        caps->parent=TRUE;
        caps->child=FALSE;
        caps->name=strdup("Main Chart");
        caps->type=strdup("gchartman/main_chart");
        caps->icon=NULL;
        return caps;
}

void free_object(ChartInstance *ci) {
	struct mainchart_data *d=(struct mainchart_data *)ci->instance_data;	// Cast is necessary...

	if (d->item!=NULL) gtk_object_destroy(GTK_OBJECT(d->item));
	if (ci->fixed_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
	g_free(d);
	g_free(ci->instance_name);
	xmlFreeNode(ci->xml_node);
}
/*
static gint main_chart_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer data) {

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
//			gnome_canvas_item_set(item,
//						"fill_color","red",
//						NULL);
			break;
		
		case GDK_LEAVE_NOTIFY:
//			gnome_canvas_item_set(item,
//						"fill_color","black",
//						NULL);
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
static int	last_mday;

static void mc_set_points(GnomeCanvasPoints *points,Quote *quote_vector,int quote_count) {
	int	i;

	mc_index=0;

	for (i=1;i<quote_count+1;i++) {

		if (quote_vector[i].valid==TRUE && quote_vector[i].interpolated==FALSE) {
			// Set X Coord
			points->coords[mc_index]=CHARTER_X(i-1);

			//Set Y Coord
			points->coords[mc_index+1]=CHARTER_Y(quote_vector[i].value);

			mc_index+=2;
		} else {
		}
	}
}

static void create_mainchart(ChartInstance *ci) {
	char	color_string[8];

	struct mainchart_data *data=(struct mainchart_data *)ci->instance_data;	// Cast is necessary...

	ci->fixed_group=NULL;
	data->item=NULL;

	if (ci->quote_count>1) {
		sprintf(color_string,"#%.2X%.2X%.2X",data->r,data->g,data->b);

		mc_lasttime=ci->quote_vector[1].timestamp;

		ci->fixed_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
				GNOME_TYPE_CANVAS_GROUP,
				"x",0.0,
				"y",0.0,
				NULL);

		mc_points=gnome_canvas_points_new(quote_count_valid);
		last_mday=-1;

		mc_set_points(mc_points,ci->quote_vector,ci->quote_count);

		data->item=gnome_canvas_item_new(ci->fixed_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",mc_points,
				"width_units",data->width,
                                "fill_color", color_string,
				NULL);	

		gnome_canvas_item_raise_to_top(data->item);

		gnome_canvas_points_free(mc_points);
		
		gnome_canvas_item_raise_to_top(GNOME_CANVAS_ITEM(ci->fixed_group));

	}
}

static void mc_update_xml_node(ChartInstance *ci) {
	struct mainchart_data *data=(struct mainchart_data *)ci->instance_data;
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
}

void create(ChartInstance *ci, xmlNodePtr config_node) {
	struct mainchart_data *data;

	ci->caps.default_create=TRUE;
	ci->caps.deleteable=FALSE;
	ci->caps.in_toolbar=FALSE;
	ci->caps.new_area=FALSE;
	ci->caps.parent=TRUE;
	ci->caps.child=FALSE;
	ci->caps.name=strdup("Main Chart");
	ci->caps.type=strdup("gchartman/main_chart");
	ci->caps.icon=NULL;
	mc_global_ci=ci;

	data=(struct mainchart_data *)malloc(sizeof(struct mainchart_data));
	ci->instance_data=data;

	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/main_chart");
	mc_update_xml_node(ci);
	if (config_node!=NULL) {
		data->r=atoi(xmlGetProp(config_node,"r"));
		data->g=atoi(xmlGetProp(config_node,"g"));
		data->b=atoi(xmlGetProp(config_node,"b"));
		data->a=atoi(xmlGetProp(config_node,"a"));
		data->width=atof(xmlGetProp(config_node,"width"));
	} else {
		data->r=0;
		data->g=0;
		data->b=0;
		data->a=0;
		data->width=2.0;
	}	

	mc_update_xml_node(ci);


	ci->move_group=NULL;
	create_mainchart(ci);

	ci->instance_name=strdup("Main Chart");

}

void on_mainchart_propertybox_apply (GnomePropertyBox *property_box, gint page_num) {
	struct mainchart_data *data=(struct mainchart_data *)mc_global_ci->instance_data;

	if (page_num != -1) return;

	gnome_color_picker_get_i8(mc_cpick,&data->r,&data->g,&data->b,&data->a);
	data->width=gtk_spin_button_get_value_as_float(mc_widthspin);

	gtk_object_destroy(GTK_OBJECT(mc_global_ci->fixed_group));

	create_mainchart(mc_global_ci);
	mc_update_xml_node(mc_global_ci);

//	g_print("Font: %s\n",data->fontname);	
//	g_print("Colors: %i %i %i %i\n",data->r,data->g,data->b,data->a);
}

void set_properties(ChartInstance *ci) {
	GnomePropertyBox *pbox;
	GladeXML *xxml;

	struct mainchart_data *data=(struct mainchart_data *)mc_global_ci->instance_data;

	xxml=glade_xml_new(MAINCHARTGLADE_FILE,"MainChartPropertyBox");
     	glade_xml_signal_autoconnect(xxml);

	pbox=(GnomePropertyBox *)glade_xml_get_widget(xxml,"MainChartPropertyBox");
	mc_cpick=(GnomeColorPicker *)glade_xml_get_widget(xxml,"MainChartColor");
	mc_widthspin=(GtkSpinButton *)glade_xml_get_widget(xxml,"MainChartWidth");

	//Hack away the "apply"  button
	gtk_widget_hide(GNOME_PROPERTY_BOX(pbox)->apply_button);

	gnome_property_box_changed(pbox);	//Necessary, or the callback won't be called

	gnome_color_picker_set_i8(mc_cpick,data->r,data->g,data->b,data->a);
	gtk_spin_button_set_value(mc_widthspin,data->width);
		
	gnome_dialog_run(GNOME_DIALOG(pbox));
	data=NULL;
}

void update(ChartInstance *ci) {
	struct mainchart_data *data=(struct mainchart_data *)mc_global_ci->instance_data;

	GnomeCanvasPoints *points;
	
	if (ci->quote_count>1) {
		points=gnome_canvas_points_new(quote_count_valid);

		mc_index=0;	

		last_mday=-1;

		mc_set_points(points,ci->quote_vector,ci->quote_count);

		gnome_canvas_item_set(data->item,
				"points",points,
				NULL);
				
		gnome_canvas_points_free(points);
	}
}

void redraw(ChartInstance *ci) {
	struct mainchart_data *data=(struct mainchart_data *)mc_global_ci->instance_data;

	GnomeCanvasPoints *points;
	
	if (ci->quote_count>1) {
		points=gnome_canvas_points_new(quote_count_valid);

		mc_index=0;	

		last_mday=-1;

		mc_set_points(points,ci->quote_vector,ci->quote_count);

		gnome_canvas_item_set(data->item,
				"points",points,
				NULL);
				
		gnome_canvas_points_free(points);
	}
}
