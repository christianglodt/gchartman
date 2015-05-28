#include "../module.h"

struct trendline_data {
	GnomeCanvasItem	*trendline;
	GnomeCanvasItem *handle1;
	GnomeCanvasItem *handle2;
	GnomeCanvasPoints *line_points;
	int	day,month,year;
	double	t1,t2,v1,v2;
	double	time_offset;
};

static int trendline_counter=0;

/*char *get_instance_name(void *_data) {
	char	*n;
	
	n=malloc(20);
	sprintf(n,"Trend line %i",trendline_counter);
	return n;
}
*/
ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=FALSE;
	caps->deleteable=TRUE;
	caps->in_toolbar=TRUE;
	caps->new_area=FALSE;
	caps->parent=FALSE;
	caps->child=FALSE;
	caps->name=strdup("Trend line");
	caps->type=strdup("gchartman/trendline");
	caps->icon=strdup("trendline.png");
	return caps;
}

void update(ChartInstance *ci) {
	struct trendline_data *data=(struct trendline_data *)ci->instance_data;

	data->line_points->coords[0]=CHARTER_X(data->t1);
	data->line_points->coords[1]=CHARTER_Y(data->v1);
	data->line_points->coords[2]=CHARTER_X(data->t2);
	data->line_points->coords[3]=CHARTER_Y(data->v2);
	
	gnome_canvas_item_set(data->trendline,
				"points",data->line_points,
				NULL);
	gnome_canvas_item_set(data->handle1,
				"x1",data->line_points->coords[0]-2.0,
				"y1",data->line_points->coords[1]-2.0,
				"x2",data->line_points->coords[0]+2.0,
				"y2",data->line_points->coords[1]+2.0,
				NULL);

	gnome_canvas_item_set(data->handle2,
				"x1",data->line_points->coords[2]-2.0,
				"y1",data->line_points->coords[3]-2.0,
				"x2",data->line_points->coords[2]+2.0,
				"y2",data->line_points->coords[3]+2.0,
				NULL);
}

void redraw(ChartInstance *ci) {
	struct trendline_data *data=(struct trendline_data *)ci->instance_data;

	data->line_points->coords[0]=CHARTER_X(data->t1);
	data->line_points->coords[1]=CHARTER_Y(data->v1);
	data->line_points->coords[2]=CHARTER_X(data->t2);
	data->line_points->coords[3]=CHARTER_Y(data->v2);
	
	gnome_canvas_item_set(data->trendline,
				"points",data->line_points,
				NULL);
	gnome_canvas_item_set(data->handle1,
				"x1",data->line_points->coords[0]-2.0,
				"y1",data->line_points->coords[1]-2.0,
				"x2",data->line_points->coords[0]+2.0,
				"y2",data->line_points->coords[1]+2.0,
				NULL);

	gnome_canvas_item_set(data->handle2,
				"x1",data->line_points->coords[2]-2.0,
				"y1",data->line_points->coords[3]-2.0,
				"x2",data->line_points->coords[2]+2.0,
				"y2",data->line_points->coords[3]+2.0,
				NULL);
}

void free_object(ChartInstance *ci) {
	struct trendline_data *data=(struct trendline_data *)ci->instance_data;

	gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
	gnome_canvas_points_free(data->line_points);

	trendline_counter--;

	g_free(data);
}

static void move_linepoint(struct trendline_data *data,double X, double Y,int pointnum) {

	data->line_points->coords[pointnum]=X;
	data->line_points->coords[pointnum+1]=Y;
	gnome_canvas_item_set(data->trendline,
				"points",data->line_points,
				NULL);
	if (pointnum==0) {
		gnome_canvas_item_set(data->handle1,
			"x1",X-2.0,
			"y1",Y-2.0,
			"x2",X+2.0,
			"y2",Y+2.0,
			NULL);
		data->t1=CHARTER_RX(X);
		data->v1=CHARTER_RY(Y);
	} else {
		gnome_canvas_item_set(data->handle2,
			"x1",X-2.0,
			"y1",Y-2.0,
			"x2",X+2.0,
			"y2",Y+2.0,
			NULL);
		data->t2=CHARTER_RX(X);
		data->v2=CHARTER_RY(Y);
	}
}

static double	press_x,press_y;

static void update_xml_node(ChartInstance *ci) {
	struct trendline_data *data=(struct trendline_data *)ci->instance_data;

	char	temp_string[20];

	sprintf(temp_string,"%g",data->t1-data->time_offset);
	xmlSetProp(ci->xml_node,"t1",temp_string);
	sprintf(temp_string,"%g",data->v1);
	xmlSetProp(ci->xml_node,"v1",temp_string);
	sprintf(temp_string,"%g",data->t2-data->time_offset);
	xmlSetProp(ci->xml_node,"t2",temp_string);
	sprintf(temp_string,"%g",data->v2);
	xmlSetProp(ci->xml_node,"v2",temp_string);
	sprintf(temp_string,"%i",data->day);
	xmlSetProp(ci->xml_node,"day",temp_string);
	sprintf(temp_string,"%i",data->month);
	xmlSetProp(ci->xml_node,"month",temp_string);
	sprintf(temp_string,"%i",data->year);
	xmlSetProp(ci->xml_node,"year",temp_string);
}

static ChartInstance *_global_ci;

static gint trendline_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer _data) {
	GdkCursor	*fleur;
	struct trendline_data *data=(struct trendline_data *)_data;

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
			gnome_canvas_item_set(data->handle1,"fill_color","#909090",NULL);
			gnome_canvas_item_set(data->handle2,"fill_color","#909090",NULL);
			break;
		
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_item_set(data->handle1,"fill_color",NULL,NULL);
			gnome_canvas_item_set(data->handle2,"fill_color",NULL,NULL);
			break;
		
		case GDK_BUTTON_PRESS:
			switch(event->button.button) {
				case 1:		// Left Mousebutton
					fleur = gdk_cursor_new (GDK_FLEUR);
					press_x=event->button.x;
					press_y=event->button.y;
					
			                gnome_canvas_item_grab (item,
        	                                GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
	                                        fleur, event->button.time);
			                gdk_cursor_destroy (fleur);

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
			gnome_canvas_item_ungrab (item, event->button.time);
			update_xml_node(_global_ci);
			break;
		
		case GDK_MOTION_NOTIFY:
			if (event->motion.state & GDK_BUTTON1_MASK){
				// Do some delta-magic to move the points relatively to pointer motion
				move_linepoint(data,
					CHARTER_X(data->t1)-(press_x-event->motion.x),
					CHARTER_Y(data->v1)-(press_y-event->motion.y),0);
				move_linepoint(data,
					CHARTER_X(data->t2)-(press_x-event->motion.x),
					CHARTER_Y(data->v2)-(press_y-event->motion.y),2);
				press_x=event->motion.x;
				press_y=event->motion.y;
			}
			break;
		default:break;
		
	}
	return FALSE;
}

static gint handle_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer _data) {

	struct trendline_data *data=(struct trendline_data *)_data;
	GdkCursor	*fleur;

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
//			gnome_canvas_item_set(item,"fill_color","red",NULL);
			gnome_canvas_item_set(data->handle1,"fill_color","black",NULL);
			gnome_canvas_item_set(data->handle2,"fill_color","black",NULL);
//			gnome_canvas_item_set(item,"outline_color","black",NULL);
			break;
		
		case GDK_LEAVE_NOTIFY:
//			gnome_canvas_item_set(item,"fill_color","black",NULL);
			gnome_canvas_item_set(data->handle1,"fill_color",NULL,NULL);
			gnome_canvas_item_set(data->handle2,"fill_color",NULL,NULL);
//			gnome_canvas_item_set(item,"outline_color",NULL,NULL);
			break;
		
		case GDK_BUTTON_PRESS:
			switch(event->button.button) {
				case 1:		// Left Mousebutton
					fleur = gdk_cursor_new (GDK_FLEUR);
			                gnome_canvas_item_grab (item,
        	                                GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
	                                        fleur, event->button.time);
			                gdk_cursor_destroy (fleur);

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
			gnome_canvas_item_ungrab (item, event->button.time);
			update_xml_node(_global_ci);
			break;
		
		case GDK_MOTION_NOTIFY:
			if (event->motion.state & GDK_BUTTON1_MASK){
				if (item==data->handle1) {
					move_linepoint(data,event->motion.x,event->motion.y,0);
				}
				if (item==data->handle2) {
					move_linepoint(data,event->motion.x,event->motion.y,2);
				}
			}
			break;
		default: break;
		
	}
	return FALSE;
}

void create(ChartInstance *ci,xmlNodePtr config_node) {
	GnomeCanvasGroup *temp_group;
	struct trendline_data *data;
//	struct tl_pdata *pdata=(struct tl_pdata *)_pdata;
	int	i;
	char	*name;
	
	_global_ci=ci;
	trendline_counter++;

	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/trendline");

	data=(struct trendline_data *)malloc(sizeof(struct trendline_data));
	ci->instance_data=data;

		if (config_node!=NULL) {
			data->t1=atof(xmlGetProp(config_node,"t1"));
			data->v1=atof(xmlGetProp(config_node,"v1"));
			data->t2=atof(xmlGetProp(config_node,"t2"));
			data->v2=atof(xmlGetProp(config_node,"v2"));
			data->day=atoi(xmlGetProp(config_node,"day"));
			data->month=atoi(xmlGetProp(config_node,"month"));
			data->year=atoi(xmlGetProp(config_node,"year"));
		
/*			data->t1=pdata->t1;
			data->v1=pdata->v1;
			data->t2=pdata->t2;
			data->v2=pdata->v2;
			data->day=pdata->day;
			data->month=pdata->month;
			data->year=pdata->year;
*/			i=1;
			while (ci->quote_vector[i].year!=data->year && i<ci->quote_count) i++;
			while (ci->quote_vector[i].month!=data->month && i<ci->quote_count) i++;
			while (ci->quote_vector[i].day!=data->day && i<ci->quote_count) i++;
			data->time_offset=i-1;
			data->t1+=data->time_offset;
			data->t2+=data->time_offset;
			
		} else {
			data->t1=CHARTER_RX(-100);
			data->v1=CHARTER_RY(-100);
			data->t2=CHARTER_RX(0);
			data->v2=CHARTER_RY(0);
			data->day=ci->quote_vector[1].day;
			data->month=ci->quote_vector[1].month;
			data->year=ci->quote_vector[1].year;
			data->time_offset=0;
		}

		temp_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
			GNOME_TYPE_CANVAS_GROUP,
			"x",0.0,
			"y",0.0,
			NULL);

		ci->fixed_group=temp_group;
	
		data->line_points=gnome_canvas_points_new(2);

		data->line_points->coords[0]=CHARTER_X(data->t1);
		data->line_points->coords[1]=CHARTER_Y(data->v1);
		data->line_points->coords[2]=CHARTER_X(data->t2);
		data->line_points->coords[3]=CHARTER_Y(data->v2);

		data->trendline=gnome_canvas_item_new(temp_group,
			GNOME_TYPE_CANVAS_LINE,
			"points",data->line_points,
			"width_units",1.0,
			"fill_color", "Blue",
/*			"smooth",TRUE,		//is this not yet implemented, or
			"spline_steps",20,	//does it only work in the aa canvas?
*/			NULL);	

		data->handle1=gnome_canvas_item_new(temp_group,
			GNOME_TYPE_CANVAS_RECT,
			"x1",data->line_points->coords[0]-2.0,
			"y1",data->line_points->coords[1]-2.0,
			"x2",data->line_points->coords[0]+2.0,
			"y2",data->line_points->coords[1]+2.0,
			"outline_color",NULL,
			"width_pixels",1,
			"fill_color",NULL,
			NULL);

		gtk_signal_connect(GTK_OBJECT(data->handle1),"event",(GtkSignalFunc)handle_callback,data);

		data->handle2=gnome_canvas_item_new(temp_group,
			GNOME_TYPE_CANVAS_RECT,
			"x1",data->line_points->coords[2]-2.0,
			"y1",data->line_points->coords[3]-2.0,
			"x2",data->line_points->coords[2]+2.0,
			"y2",data->line_points->coords[3]+2.0,
			"outline_color",NULL,
			"width_pixels",1,
			"fill_color",NULL,
			NULL);

		gtk_signal_connect(GTK_OBJECT(data->handle2),"event",(GtkSignalFunc)handle_callback,data);

		gtk_signal_connect(GTK_OBJECT(data->trendline),"event",(GtkSignalFunc)trendline_callback,data);
		update_xml_node(ci);
	
		name=malloc(20);
		sprintf(name,"Trend line %i",trendline_counter);
		ci->instance_name=name;

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
