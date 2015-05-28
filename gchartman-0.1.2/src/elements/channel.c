#include "../module.h"
#include <math.h>

struct channel_data {
	GnomeCanvasGroup *group;
	GnomeCanvasItem	*line1;
	GnomeCanvasItem	*line2;
	GnomeCanvasItem	*line3;
	GnomeCanvasItem *handle1;
	GnomeCanvasItem *handle2;
	GnomeCanvasItem *handle3;
	GnomeCanvasPoints *line1_points;
	GnomeCanvasPoints *line2_points;
	GnomeCanvasPoints *line3_points;
	double	t1,v1,t2,v2,dist;
	double	old_cx,old_cy;
	double	time_offset;
	int	day,month,year;
};

struct channel_pdata {
	int	day,month,year;
	double t1,v1,t2,v2,dist;
};

static int channel_counter=0;

char *get_instance_name(void *_data) {
	char	*n;
	
	n=malloc(20);
	sprintf(n,"Channel %i",channel_counter);
	return n;
}

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=FALSE;
	caps->deleteable=TRUE;
	caps->in_toolbar=TRUE;
	caps->new_area=FALSE;
	caps->parent=FALSE;
	caps->child=FALSE;
	caps->name=strdup("Channel");
	caps->type=strdup("gchartman/channel");
	caps->icon=strdup("channel.png");
	return caps;
}

GnomeCanvasGroup *get_group(void *data,int size_resp) {
	struct channel_data *d=(struct channel_data *)data;	// Cast is necessary...
	
	return d->group;
}

void free_object(void *data) {
	struct channel_data *d=(struct channel_data *)data;	// Cast is necessary...

	gtk_object_destroy(GTK_OBJECT(d->line1));
	gtk_object_destroy(GTK_OBJECT(d->line2));
	gtk_object_destroy(GTK_OBJECT(d->line3));
	gtk_object_destroy(GTK_OBJECT(d->handle1));
	gtk_object_destroy(GTK_OBJECT(d->handle2));
	gtk_object_destroy(GTK_OBJECT(d->handle3));
	gtk_object_destroy(GTK_OBJECT(d->group));
	gnome_canvas_points_free(d->line1_points);
	gnome_canvas_points_free(d->line2_points);
	gnome_canvas_points_free(d->line3_points);

	channel_counter--;

	g_free(d);
}

/*
 *	This function resets all lines & handles according to t1,v1,t2,v2
 */

void reset_from_data(struct channel_data *data) {
	double	xoffset,yoffset,angle;
	double	xoffset2,yoffset2;
	double	distfact,xdistfact,ydistfact;
				
	data->line1_points->coords[0]=CHARTER_X(data->t1);
	data->line1_points->coords[1]=CHARTER_Y(data->v1);
	data->line1_points->coords[2]=CHARTER_X(data->t2);
	data->line1_points->coords[3]=CHARTER_Y(data->v2);

	angle=acos((CHARTER_X(data->t1-data->t2)*1+CHARTER_Y(data->v1-data->v2)*0)/sqrt(pow(CHARTER_X(data->t2-data->t1),2)+pow(CHARTER_Y(data->v2-data->v1),2)));
	if (data->v1 < data->v2) angle= -angle;

	// Initial Rotation
	// FIXME: Need to take CHARTER_Y into account cause scaling is not working well
	// Might need to save Charter_X/Y

	xdistfact=(data->old_cx/CHARTER_X(1));
	ydistfact=(data->old_cy/CHARTER_Y(1));

	distfact=ydistfact/xdistfact;

//FIXME: Find a better way once and for all

	xoffset=CHARTER_X(data->dist*xdistfact);
	yoffset=CHARTER_Y((data->dist*ydistfact));
//	xoffset=CHARTER_X(data->dist*xdistfact*cos(angle));
//	yoffset=CHARTER_Y((data->dist*ydistfact)*sin(angle));
	printf("Zoom: %f,%f,%f\n",xdistfact,ydistfact,distfact);

	// Rotate by another 90°
	xoffset2=-((xoffset*cos(3.1415/2))-(yoffset*sin(3.1415/2)));
	yoffset2=((xoffset*sin(3.1415/2))+(yoffset*cos(3.1415/2)));
	printf("Offs: %f,%f\n",xoffset2,yoffset2);

	data->line3_points->coords[0]=CHARTER_X((data->t1+data->t2)/2);
	data->line3_points->coords[1]=CHARTER_Y((data->v1+data->v2)/2);
	data->line3_points->coords[2]=CHARTER_X(((data->t1+data->t2)/2))+xoffset2;
	data->line3_points->coords[3]=CHARTER_Y(((data->v1+data->v2)/2))+yoffset2;

	data->line2_points->coords[0]=CHARTER_X(data->t1)+xoffset2;
	data->line2_points->coords[1]=CHARTER_Y(data->v1)+yoffset2;
	data->line2_points->coords[2]=CHARTER_X(data->t2)+xoffset2;
	data->line2_points->coords[3]=CHARTER_Y(data->v2)+yoffset2;
	gnome_canvas_item_set(data->line1,"points",data->line1_points,NULL);
	gnome_canvas_item_set(data->line2,"points",data->line2_points,NULL);
	gnome_canvas_item_set(data->line3,"points",data->line3_points,NULL);

	gnome_canvas_item_set(data->handle3,
		"x1",data->line3_points->coords[2]-2.0,
		"y1",data->line3_points->coords[3]-2.0,
		"x2",data->line3_points->coords[2]+2.0,
		"y2",data->line3_points->coords[3]+2.0,
		NULL);

	gnome_canvas_item_set(data->handle1,
		"x1",data->line1_points->coords[0]-2.0,
		"y1",data->line1_points->coords[1]-2.0,
		"x2",data->line1_points->coords[0]+2.0,
		"y2",data->line1_points->coords[1]+2.0,
		NULL);

	gnome_canvas_item_set(data->handle2,
		"x1",data->line1_points->coords[2]-2.0,
		"y1",data->line1_points->coords[3]-2.0,
		"x2",data->line1_points->coords[2]+2.0,
		"y2",data->line1_points->coords[3]+2.0,
		NULL);
}

void rezoom(struct channel_data *data,Quote *quote_vector,int quote_counter) {
	reset_from_data(data);
}

double	press_x,press_y;

static gint channel_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer _data) {
	GdkCursor	*fleur;
	struct channel_data *data=(struct channel_data *)_data;
	
	switch(event->type) {
		case GDK_ENTER_NOTIFY:
			gnome_canvas_item_set(data->handle1,"fill_color","#909090",NULL);
			gnome_canvas_item_set(data->handle2,"fill_color","#909090",NULL);
			gnome_canvas_item_set(data->handle3,"fill_color","#909090",NULL);
			break;
		
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_item_set(data->handle1,"fill_color",NULL,NULL);
			gnome_canvas_item_set(data->handle2,"fill_color",NULL,NULL);
			gnome_canvas_item_set(data->handle3,"fill_color",NULL,NULL);
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
			break;
		
		case GDK_MOTION_NOTIFY:
			if (event->motion.state & GDK_BUTTON1_MASK){
				// Do some delta-magic to move the points relatively to pointer motion

				data->t1-=CHARTER_RX(press_x-event->motion.x);
				data->v1-=CHARTER_RY(press_y-event->motion.y);
				data->t2-=CHARTER_RX(press_x-event->motion.x);
				data->v2-=CHARTER_RY(press_y-event->motion.y);
				reset_from_data(data);

				press_x=event->motion.x;
				press_y=event->motion.y;
			}
			break;
		default:break;
		
	}
	return FALSE;
}

static gint handle_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer _data) {
	struct channel_data *data=(struct channel_data *)_data;
	GdkCursor	*fleur;

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
			gnome_canvas_item_set(data->handle1,"fill_color","black",NULL);
			gnome_canvas_item_set(data->handle2,"fill_color","black",NULL);
			gnome_canvas_item_set(data->handle3,"fill_color","black",NULL);
			break;
		
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_item_set(data->handle1,"fill_color",NULL,NULL);
			gnome_canvas_item_set(data->handle2,"fill_color",NULL,NULL);
			gnome_canvas_item_set(data->handle3,"fill_color",NULL,NULL);
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
			break;
		
		case GDK_MOTION_NOTIFY:
			if (event->motion.state & GDK_BUTTON1_MASK){
				if (item==data->handle1) {
					data->t1=CHARTER_RX(event->motion.x);
					data->v1=CHARTER_RY(event->motion.y);
					reset_from_data(data);
				}
				if (item==data->handle2) {
					data->t2=CHARTER_RX(event->motion.x);
					data->v2=CHARTER_RY(event->motion.y);
					reset_from_data(data);
				}
				if (item==data->handle3) {
					// Calc new dist
				
					data->dist=-CHARTER_RX(sqrt(pow((press_x-data->line3_points->coords[0]),2)+pow((press_y-data->line3_points->coords[1]),2)));
					reset_from_data(data);
				}

				press_x=event->motion.x;
				press_y=event->motion.y;
			}
			break;
		default: break;
		
	}
	return FALSE;
}

void *get_pdata(void *_data,int *len) {
	struct channel_data *data=(struct channel_data *)_data;
	struct channel_pdata *pdata;
	
	pdata=(struct channel_pdata *)malloc(sizeof(struct channel_pdata));

	pdata->t1=data->t1;
	pdata->t2=data->t2;
	pdata->v1=data->v1;
	pdata->v2=data->v2;
	pdata->day=data->day;
	pdata->month=data->month;
	pdata->year=data->year;
	pdata->dist=data->dist;

	*len=sizeof(*pdata);
	return pdata;
}

xmlNodePtr get_xml_node(void *_data) {
	struct channel_data *data=(struct channel_data *)_data;
	xmlNodePtr	node;
	char	temp_string[20];

	node=xmlNewNode(NULL,"item");
	xmlSetProp(node,"type","gchartman/channel");
	sprintf(temp_string,"%g",data->t1);
	xmlSetProp(node,"t1",temp_string);
	sprintf(temp_string,"%g",data->v1);
	xmlSetProp(node,"v1",temp_string);
	sprintf(temp_string,"%g",data->t2);
	xmlSetProp(node,"t2",temp_string);
	sprintf(temp_string,"%g",data->v2);
	xmlSetProp(node,"v2",temp_string);
	sprintf(temp_string,"%i",data->day);
	xmlSetProp(node,"day",temp_string);
	sprintf(temp_string,"%i",data->month);
	xmlSetProp(node,"month",temp_string);
	sprintf(temp_string,"%i",data->year);
	xmlSetProp(node,"year",temp_string);
	sprintf(temp_string,"%g",data->dist);
	xmlSetProp(node,"dist",temp_string);
	
	return node;
}


void *create_new(Quote *quote_vector,int *quote_counter,xmlNodePtr node) {
	GnomeCanvasGroup *temp_group;
	struct channel_data *data;
//	struct channel_pdata *pdata=(struct channel_pdata *)_pdata;
	int	i;
	
	channel_counter++;

	data=(struct channel_data *)malloc(sizeof(struct channel_data));

		if (node!=NULL) {
			data->t1=atof(xmlGetProp(node,"t1"));
			data->v1=atof(xmlGetProp(node,"v1"));
			data->t2=atof(xmlGetProp(node,"t2"));
			data->v2=atof(xmlGetProp(node,"v2"));
			
			data->dist=atof(xmlGetProp(node,"dist"));

			data->old_cx=CHARTER_X(1);
			data->old_cy=CHARTER_Y(1);

			data->day=atoi(xmlGetProp(node,"day"));
			data->month=atoi(xmlGetProp(node,"month"));
			data->year=atoi(xmlGetProp(node,"year"));
			i=1;
			while (quote_vector[i].year!=data->year && i<*quote_counter) i++;
			while (quote_vector[i].month!=data->month && i<*quote_counter) i++;
			while (quote_vector[i].day!=data->day && i<*quote_counter) i++;
			data->time_offset=i-1;
			data->t1+=data->time_offset;
			data->t2+=data->time_offset;
		} else {
			data->t1=CHARTER_RX(-100);
			data->v1=CHARTER_RY(-100);
			data->t2=CHARTER_RX(0);
			data->v2=CHARTER_RY(0);
			data->dist=10;
			data->old_cx=CHARTER_X(1);
			data->old_cy=CHARTER_Y(1);
			data->day=quote_vector[1].day;
			data->month=quote_vector[1].month;
			data->year=quote_vector[1].year;
			data->time_offset=0;
		}

		temp_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
				GNOME_TYPE_CANVAS_GROUP,
				"x",0.0,
				"y",0.0,
				NULL);

		data->group=temp_group;
	
		data->line1_points=gnome_canvas_points_new(2);
		data->line3_points=gnome_canvas_points_new(2);
		data->line2_points=gnome_canvas_points_new(2);

		data->line1=gnome_canvas_item_new(temp_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",data->line1_points,
				"width_units",1.0,
                                "fill_color", "Blue",
				NULL);	

		data->line2=gnome_canvas_item_new(temp_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",data->line2_points,
				"width_units",1.0,
                                "fill_color", "Blue",
				NULL);	

		data->line3=gnome_canvas_item_new(temp_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",data->line3_points,
				"width_units",0.5,
                                "fill_color", "#909090",
				NULL);	

		data->handle1=gnome_canvas_item_new(temp_group,
				GNOME_TYPE_CANVAS_RECT,
				"x1",CHARTER_X(data->t1)-2.0,
				"y1",CHARTER_Y(data->v1)-2.0,
				"x2",CHARTER_X(data->t1)+2.0,
				"y2",CHARTER_Y(data->v1)+2.0,
				"outline_color",NULL,
				"width_pixels",1,
				"fill_color",NULL,
				NULL);

		gtk_signal_connect(GTK_OBJECT(data->handle1),"event",(GtkSignalFunc)handle_callback,data);

		data->handle2=gnome_canvas_item_new(temp_group,
				GNOME_TYPE_CANVAS_RECT,
				"x1",CHARTER_X(data->t2)-2.0,
				"y1",CHARTER_Y(data->v2)-2.0,
				"x2",CHARTER_X(data->t2)+2.0,
				"y2",CHARTER_Y(data->v2)+2.0,
				"outline_color",NULL,
				"width_pixels",1,
				"fill_color",NULL,
				NULL);

		gtk_signal_connect(GTK_OBJECT(data->handle2),"event",(GtkSignalFunc)handle_callback,data);

		data->handle3=gnome_canvas_item_new(temp_group,
				GNOME_TYPE_CANVAS_RECT,
				"x1",data->line3_points->coords[2]-2.0,
				"y1",data->line3_points->coords[3]-2.0,
				"x2",data->line3_points->coords[2]+2.0,
				"y2",data->line3_points->coords[3]+2.0,
				"outline_color",NULL,
				"width_pixels",1,
				"fill_color",NULL,
				NULL);

		gtk_signal_connect(GTK_OBJECT(data->handle3),"event",(GtkSignalFunc)handle_callback,data);

		gtk_signal_connect(GTK_OBJECT(data->line1),"event",(GtkSignalFunc)channel_callback,data);
		gtk_signal_connect(GTK_OBJECT(data->line2),"event",(GtkSignalFunc)channel_callback,data);
		gtk_signal_connect(GTK_OBJECT(data->line3),"event",(GtkSignalFunc)channel_callback,data);

		reset_from_data(data);

	return data;
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
