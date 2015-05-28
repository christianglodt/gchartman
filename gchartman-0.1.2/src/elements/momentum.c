#include "../module.h"
#include <math.h>

#define	MOMENTUMGLADE_FILE PLUGINDIR"/momentumprop.glade"

struct momentum_data {
	GnomeCanvasItem	*line;
	GnomeCanvasItem	*line2;
	GnomeCanvasItem	*rect;
	GnomeCanvasItem	*text;
	GnomeCanvasItem	*text2;
	GnomeCanvasItem	*text3;
	GnomeCanvasItem	*text4;
	
	guint8	r,g,b,a;
	gfloat	width;
	int	days;
};
/*
struct momentum_pdata {
	guint8	r,g,b,a;
	float	width;
	int	days;
};
*/
static int	momentum_index;
static time_t	momentum_lasttime;

static GnomeCanvasPoints *momentum_points;

static GnomeColorPicker *momentum_cpick;
static GtkSpinButton *momentum_widthspin;
static GtkSpinButton *momentum_daysspin;

static ChartInstance *_global_ci;	//Only for use in PropertyBox handlers !!!

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=FALSE;
	caps->deleteable=TRUE;
	caps->in_toolbar=TRUE;
	caps->new_area=TRUE;
	caps->parent=TRUE;
	caps->child=TRUE;
	caps->name=strdup("Momentum");
	caps->type=strdup("gchartman/momentum");
	caps->icon=strdup("momentum.png");
	return caps;
}

static void update_xml_node(ChartInstance *ci) {
	struct momentum_data *data=(struct momentum_data *)ci->instance_data;
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
//	xmlElemDump(0,NULL,ci->xml_node);
//	printf("\n");
}

static void update_momentum_label(ChartInstance *ci) {
	char momentum_name[512];
	struct momentum_data *data=(struct momentum_data *)ci->instance_data;

	if (ci->parent) {
		sprintf(momentum_name,"Momentum(%i) of %s",data->days,ci->parent->instance_name);
		gnome_canvas_item_set(data->text4,"text",strdup(momentum_name),NULL);
	} else {
		sprintf(momentum_name,"Momentum(%i)",data->days);
		gnome_canvas_item_set(data->text4,"text",strdup(momentum_name),NULL);
	}

}

static void update_instance_name(ChartInstance *ci) {
	char momentum_name[30];
	struct momentum_data *data=(struct momentum_data *)ci->instance_data;

	sprintf(momentum_name,"Momentum (%i)",data->days);
	ci->instance_name=strdup(momentum_name);
}

void free_object(ChartInstance *ci) {
	struct momentum_data *d=(struct momentum_data *)ci->instance_data;

	gtk_object_destroy(GTK_OBJECT(d->line));
	gtk_object_destroy(GTK_OBJECT(d->line2));
	gtk_object_destroy(GTK_OBJECT(d->rect));
	gtk_object_destroy(GTK_OBJECT(d->text));
	gtk_object_destroy(GTK_OBJECT(d->text2));
	gtk_object_destroy(GTK_OBJECT(d->text3));
	gtk_object_destroy(GTK_OBJECT(d->text4));
	gtk_object_destroy(GTK_OBJECT(ci->move_group));
	g_free(d);
}
/*
static gint momentum_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer data) {

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
static int	momentum_last_mday;

static double	momentum_max;

static void momentum_set_points(GnomeCanvasPoints *points,const Quote *quote_vector,int quote_count,int days,Quote *new_qv,int *new_qc) {
	int	i;
	double	temp[quote_count-days];
	int	tempindex=0;

	momentum_max=0;

	// First Coord is newest

	momentum_index=0;

	for (i=1;i<(quote_count-days)+1;i++) {

		points->coords[momentum_index]=CHARTER_X(i-1);

		points->coords[momentum_index+1]=-(quote_vector[i].value-quote_vector[i+days].value);

		if (fabs(points->coords[momentum_index+1])>momentum_max)
			momentum_max=fabs(points->coords[momentum_index+1]);

		momentum_index+=2;
	}
	
	// scale to a range of 100 pixels
	
	momentum_index=0;

	for (i=1;i<(quote_count-days)+1;i++) {

		points->coords[momentum_index+1]*=(50/momentum_max);
		points->coords[momentum_index+1]-=50;

		temp[tempindex]=CHARTER_RY(points->coords[momentum_index+1]);
		tempindex++;
		momentum_index+=2;
	}

//	if (modify_qv) {
		for (i=0;i<(quote_count-days);i++) {
			new_qv[i].value=temp[i];
		}

		*new_qc=quote_count-days;
//	}

//	new_count=*quote_count-days;
}

static void create_momentum(ChartInstance *ci) {
	struct momentum_data *data=(struct momentum_data *)ci->instance_data;
	char	color_string[8];
	char	text_string[20];

	if ((ci->quote_count)>1 && (ci->quote_count)>(data->days+1)) {
		sprintf(color_string,"#%.2X%.2X%.2X",data->r,data->g,data->b);

		momentum_lasttime=ci->quote_vector[1].timestamp;

		ci->move_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
				GNOME_TYPE_CANVAS_GROUP,
				"x",0.0,
				"y",0.0,
				NULL);

		momentum_points=gnome_canvas_points_new((ci->quote_count - (data->days)));

		momentum_last_mday=-1;

		momentum_set_points(momentum_points,ci->quote_vector,ci->quote_count,data->days,ci->new_quote_vector,&ci->new_quote_count);

		data->line=gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",momentum_points,
				"width_units",data->width,
                                "fill_color", color_string,
				NULL);	

		gnome_canvas_points_free(momentum_points);

		data->rect=gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_RECT,
				"x1",1.0*CHARTER_X(ci->quote_count-1),
				"y1",-100.0,
				"x2",0.0,
				"y2",0.0,
				"fill_color",NULL,
				"outline_color","black",
				"width_pixels",1,
				NULL);	

		momentum_points=gnome_canvas_points_new(2);

		momentum_points->coords[0]=1.0*CHARTER_X(ci->quote_count-1);
		momentum_points->coords[1]=-50.0;
		momentum_points->coords[2]=0.0;
		momentum_points->coords[3]=-50.0;
		
		data->line2=gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",momentum_points,
				"width_units",1.0,
                                "fill_color", "black",
				NULL);	

		gnome_canvas_points_free(momentum_points);

		sprintf(text_string,"%.1f",momentum_max);
		data->text=gnome_canvas_item_new (ci->move_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", 5.0+1.0,
                               "y", -100.0,
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_NW,
                               "fill_color", "black",
                               NULL);

		sprintf(text_string,"%.1f",momentum_max/2);
		data->text2=gnome_canvas_item_new (ci->move_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", 5.0+1.0,
                               "y", -75.0,
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_W,
                               "fill_color", "black",
                               NULL);

		sprintf(text_string,"%.1f",0.0);
		data->text3=gnome_canvas_item_new (ci->move_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", 5.0+1.0,
                               "y", -50.0,
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_W,
                               "fill_color", "black",
                               NULL);

		sprintf(text_string,"momentum(%i)",data->days);
		data->text4=gnome_canvas_item_new (ci->move_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",text_string,
                               "x", -1.0,
                               "y", -100.0,
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_NE,
                               "fill_color", "black",
                               NULL);

		update_momentum_label(ci);				
	}
}

void create(ChartInstance *ci,xmlNodePtr config_node) {
//	struct momentum_pdata *pdata=(struct momentum_pdata *)_pdata;

	struct momentum_data *data;
	ci->instance_data=malloc(sizeof(struct momentum_data));
	data=(struct momentum_data *)ci->instance_data;
	
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
		data->r=145;
		data->g=0;
		data->b=38;
		data->a=0;
		data->width=1.5;
		data->days=14;
	}	

	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/momentum");

	update_instance_name(ci);
	update_xml_node(ci);
	create_momentum(ci);

}

/*void *get_pdata(void *_data,int *len) {
	struct momentum_data *data=(struct momentum_data *)_data;

	struct momentum_pdata *pdata=(struct momentum_pdata *)malloc(sizeof(struct momentum_pdata));
	
	pdata->r=data->r;
	pdata->g=data->g;
	pdata->b=data->b;
	pdata->a=data->a;
	
	pdata->width=data->width;
	
	pdata->days=data->days;
	
	*len=sizeof(*pdata);
	
	return pdata;
}*/

void on_momentum_propertybox_apply (GnomePropertyBox *property_box, gint page_num) {
	struct momentum_data *data=(struct momentum_data *)_global_ci->instance_data;

	if (page_num != -1) return;

	gnome_color_picker_get_i8(momentum_cpick,&data->r,&data->g,&data->b,&data->a);
	data->width=gtk_spin_button_get_value_as_float(momentum_widthspin);
	data->days=gtk_spin_button_get_value_as_int(momentum_daysspin);
	gtk_object_destroy(GTK_OBJECT(_global_ci->move_group));

	update_momentum_label(_global_ci);
	update_instance_name(_global_ci);
	update_xml_node(_global_ci);
	create_momentum(_global_ci);
	_global_ci->y_offset=0.0;
}

void set_properties(ChartInstance *ci) {
	GnomePropertyBox *momentum_pbox;
	GladeXML *xxml;

	struct momentum_data *data=(struct momentum_data *)ci->instance_data;
	
	xxml=glade_xml_new(MOMENTUMGLADE_FILE,"MomentumPropertyBox");
      	glade_xml_signal_autoconnect(xxml);

	momentum_pbox=(GnomePropertyBox *)glade_xml_get_widget(xxml,"MomentumPropertyBox");
	momentum_cpick=(GnomeColorPicker *)glade_xml_get_widget(xxml,"MomentumColor");
	momentum_widthspin=(GtkSpinButton *)glade_xml_get_widget(xxml,"MomentumWidth");
	momentum_daysspin=(GtkSpinButton *)glade_xml_get_widget(xxml,"MomentumDays");

	//Hack away the "apply"  button
	gtk_widget_hide(GNOME_PROPERTY_BOX(momentum_pbox)->apply_button);

	gnome_property_box_changed(momentum_pbox);	//Necessary, or the callback won't be called

	gnome_color_picker_set_i8(momentum_cpick,data->r,data->g,data->b,data->a);
	gtk_spin_button_set_value(momentum_widthspin,data->width);
	gtk_spin_button_set_value(momentum_daysspin,data->days);
	_global_ci=ci;
	gnome_dialog_run(GNOME_DIALOG(momentum_pbox));
	_global_ci=NULL;
}

void update(ChartInstance *ci) {
	struct momentum_data *data=(struct momentum_data *)ci->instance_data;
	gtk_object_destroy(GTK_OBJECT(data->line));
	gtk_object_destroy(GTK_OBJECT(data->line2));
	gtk_object_destroy(GTK_OBJECT(data->rect));
	gtk_object_destroy(GTK_OBJECT(data->text));
	gtk_object_destroy(GTK_OBJECT(data->text2));
	gtk_object_destroy(GTK_OBJECT(data->text3));
	gtk_object_destroy(GTK_OBJECT(data->text4));
	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));

	create_momentum(ci);
}

void redraw(ChartInstance *ci) {
	struct momentum_data *data=(struct momentum_data *)ci->instance_data;
	gtk_object_destroy(GTK_OBJECT(data->line));
	gtk_object_destroy(GTK_OBJECT(data->line2));
	gtk_object_destroy(GTK_OBJECT(data->rect));
	gtk_object_destroy(GTK_OBJECT(data->text));
	gtk_object_destroy(GTK_OBJECT(data->text2));
	gtk_object_destroy(GTK_OBJECT(data->text3));
	gtk_object_destroy(GTK_OBJECT(data->text4));
	if (ci->move_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->move_group));

	create_momentum(ci);
}

/* Line item for the canvas.  This is a polyline with configurable width, cap/join styles, and arrowheads.
 * If arrowheads are enabled, then three values are used to specify their shape:
 *
 *      arrow_shape_a:  Distance from tip of arrowhead to the center point.
 *      arrow_shape_b:  Distance from tip of arrowhead to trailing point, measured along the shaft.
 *      arrow_shape_c:  Distance of trailing point from outside edge of shaft.
 *
 * The following object arguments are available:
 *
 * name                 type                    read/write      description
 * ------------------------------------------------------------------------------------------
 * points               GnomeCanvasPoints*      RW              Pointer to a GnomeCanvasPoints structure.
 *                                                              This can be created by a call to
 *                                                              gnome_canvas_points_new() (in gnome-canvas-util.h).
 *                                                              X coordinates are in the even indices of the
 *                                                              points->coords array, Y coordinates are in
 *                                                              the odd indices.
 * fill_color           string                  W               X color specification for line
 * fill_color_gdk       GdkColor*               RW              Pointer to an allocated GdkColor
 * fill_stipple         GdkBitmap*              RW              Stipple pattern for the line
 * width_pixels         uint                    R               Width of the line in pixels.  The line width
 *                                                              will not be scaled when the canvas zoom factor changes.
 * width_units          double                  R               Width of the line in canvas units.  The line width
 *                                                              will be scaled when the canvas zoom factor changes.
 * cap_style            GdkCapStyle             RW              Cap ("endpoint") style for the line.
 * join_style           GdkJoinStyle            RW              Join ("vertex") style for the line.
 * line_style           GdkLineStyle            RW              Line dash style
 * first_arrowhead      boolean                 RW              Specifies whether to draw an arrowhead on the
 *                                                              first point of the line.
 * last_arrowhead       boolean                 RW              Specifies whether to draw an arrowhead on the
 *                                                              last point of the line.
 * smooth               boolean                 RW              Specifies whether to smooth the line using
 *                                                              parabolic splines.
 * spline_steps         uint                    RW              Specifies the number of steps to use when rendering curves.
 * arrow_shape_a        double                  RW              First arrow shape specifier.
 * arrow_shape_b        double                  RW              Second arrow shape specifier.
 * arrow_shape_c        double                  RW              Third arrow shape specifier.
 */
