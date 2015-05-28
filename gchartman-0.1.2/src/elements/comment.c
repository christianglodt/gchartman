#include "../module.h"

#define	COMMENTGLADE_FILE PLUGINDIR"/commentprop.glade"

struct comment_data {
	GnomeCanvasItem	*arrow;
	GnomeCanvasItem	*textitem;
	GnomeCanvasItem	*handle;

	char	comment[1024];
	
	double	t1,v1,t2,v2;
	double	time_offset;
	int	day,month,year;
	int	arrowenable;
};

/*struct comment_persistent_data {
	char	comment[1024];
	double	t1,v1,t2,v2;
	int	day,month,year;
	int	arrow;
};*/

//static GnomeColorPicker *cpick;
static GtkWidget *arrowbutton;
static GtkText *commenttext;

static ChartInstance *_global_ci;	//Only for use in PropertyBox handlers !!!

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=FALSE;
	caps->deleteable=TRUE;
	caps->in_toolbar=TRUE;
	caps->new_area=FALSE;
	caps->parent=FALSE;
	caps->child=FALSE;
	caps->name=strdup("Comment");
	caps->type=strdup("gchartman/comment");
	caps->icon=strdup("comment.png");
	return caps;
}

static void update_xml_node(ChartInstance *ci) {
	struct comment_data *data=(struct comment_data *)ci->instance_data;
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
	sprintf(temp_string,"%i",data->arrowenable);
	xmlSetProp(ci->xml_node,"arrow",temp_string);
//	xmlSetProp(node,"text",data->comment);
	xmlNodeSetContent(ci->xml_node,data->comment);
}

/*char *get_instance_name(void *_data) {
	return strdup("Comment");	
}*/

/*GnomeCanvasGroup *get_group(void *data,int size_resp) {
	struct comment_data *d=(struct comment_data *)data;	/
	
	return d->group;
}*/

void free_object(ChartInstance *ci) {
	struct comment_data *d=(struct comment_data *)ci->instance_data;

//	if (d->arrow!=NULL) gtk_object_destroy(GTK_OBJECT(d->arrow));
//	if (d->textitem!=NULL) gtk_object_destroy(GTK_OBJECT(d->textitem));
	if (ci->fixed_group!=NULL) gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
	ci->fixed_group=NULL;
	xmlFreeNode(ci->xml_node);
	ci->xml_node=NULL;
	g_free(d);
}

static void adjust_arrow_points(GnomeCanvasPoints *points,GnomeCanvasItem *textitem) {
	double	x1,y1,x2,y2;
	
	gnome_canvas_item_get_bounds(textitem,&x1,&y1,&x2,&y2);
	if (points->coords[0]<x1) points->coords[2]=x1;	// Left
	if (points->coords[0]>x2) points->coords[2]=x2;	// Right
	if (points->coords[1]<y1) points->coords[3]=y1;	// Above
	if (points->coords[1]>y2) points->coords[3]=y2;	// Below
}

static gint arrow_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer _ci) {
	ChartInstance *ci=(ChartInstance *)_ci;
	struct comment_data *data=(struct comment_data *)ci->instance_data;

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
			gnome_canvas_item_set(data->handle,"fill_color","#909090",NULL);
			break;
		
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_item_set(data->handle,"fill_color",NULL,NULL);
			break;
		
		case GDK_BUTTON_PRESS:
			break;
		
		case GDK_BUTTON_RELEASE:
//			gnome_canvas_item_ungrab (item, event->button.time);
			break;
		
		case GDK_MOTION_NOTIFY:
			break;
		default:
			break;
		
	}
	return FALSE;
}

static gint handle_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer _ci) {
	ChartInstance *ci=(ChartInstance *)_ci;
	struct comment_data *data=(struct comment_data *)ci->instance_data;
	
	GdkCursor	*fleur;
	GnomeCanvasPoints *comment_points;

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
			gnome_canvas_item_set(data->handle,"fill_color","black",NULL);
			break;
		
		case GDK_LEAVE_NOTIFY:
			gnome_canvas_item_set(data->handle,"fill_color",NULL,NULL);
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
			update_xml_node(ci);
			break;
		
		case GDK_MOTION_NOTIFY:
			if (event->motion.state & GDK_BUTTON1_MASK){
				data->t2=CHARTER_RX(event->motion.x);
				data->v2=CHARTER_RY(event->motion.y);
				gnome_canvas_item_set(data->handle,
					"x1",event->motion.x-2.0,
					"y1",event->motion.y-2.0,
					"x2",event->motion.x+2.0,
					"y2",event->motion.y+2.0,
					NULL);

				comment_points=gnome_canvas_points_new(2);
				comment_points->coords[0]=event->motion.x;
				comment_points->coords[1]=event->motion.y;
				comment_points->coords[2]=CHARTER_X(data->t1);
				comment_points->coords[3]=CHARTER_Y(data->v1);
				data->t2=CHARTER_RX(event->motion.x);
				data->v2=CHARTER_RY(event->motion.y);
				adjust_arrow_points(comment_points,data->textitem);
				gnome_canvas_item_set(data->arrow,
					"points",comment_points,
					NULL);

				gnome_canvas_points_free(comment_points);
			}
			break;
		default:
			break;
		
	}
	return FALSE;
}

static gint Comment_callback (GnomeCanvasItem *item, GdkEvent *event, gpointer _ci) {
	ChartInstance *ci=(ChartInstance *)_ci;
	struct comment_data *data=(struct comment_data *)ci->instance_data;
	GdkCursor	*fleur;
	GnomeCanvasPoints *comment_points;

	switch(event->type) {
		case GDK_ENTER_NOTIFY:
			break;
		
		case GDK_LEAVE_NOTIFY:
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
			update_xml_node(ci);
			break;
		
		case GDK_MOTION_NOTIFY:
			if (event->motion.state & GDK_BUTTON1_MASK){
				data->t1=CHARTER_RX(event->motion.x);
				data->v1=CHARTER_RY(event->motion.y);
				gnome_canvas_item_set(data->textitem,
					"x",event->motion.x,
					"y",event->motion.y,
					NULL);
				if (data->arrowenable) {
					comment_points=gnome_canvas_points_new(2);
					comment_points->coords[0]=CHARTER_X(data->t2);
					comment_points->coords[1]=CHARTER_Y(data->v2);
					comment_points->coords[2]=event->motion.x;
					comment_points->coords[3]=event->motion.y;
					adjust_arrow_points(comment_points,data->textitem);
					gnome_canvas_item_set(data->arrow,
						"points",comment_points,
						NULL);
					gnome_canvas_points_free(comment_points);
				}
				gnome_canvas_item_request_update(data->textitem);
			}
			break;
		default:
			break;
		
	}
	return FALSE;
}

static void create_comment(ChartInstance *ci) {
	char	color_string[8];
	GnomeCanvasPoints *comment_points;
	struct comment_data *data=(struct comment_data *)ci->instance_data;

	ci->fixed_group=NULL;
	data->arrow=NULL;

	if (ci->quote_count>1) {

		ci->fixed_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
				GNOME_TYPE_CANVAS_GROUP,
				"x",0.0,
				"y",0.0,
				NULL);
		
		data->textitem=gnome_canvas_item_new (ci->fixed_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",data->comment,
                               "x", CHARTER_X(data->t1),
                               "y", CHARTER_Y(data->v1),
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_CENTER,
                               "fill_color", "black",
                               NULL);
		
		gtk_signal_connect(GTK_OBJECT(data->textitem),"event",(GtkSignalFunc)Comment_callback,ci);

		if (data->arrowenable) {
			comment_points=gnome_canvas_points_new(2);

			data->handle=gnome_canvas_item_new(ci->fixed_group,
				GNOME_TYPE_CANVAS_RECT,
				"x1",CHARTER_X(data->t2)-2.0,
				"y1",CHARTER_Y(data->v2)-2.0,
				"x2",CHARTER_X(data->t2)+2.0,
				"y2",CHARTER_Y(data->v2)+2.0,
				"outline_color",NULL,
				"width_pixels",1,
				"fill_color",NULL,
				NULL);

			gnome_canvas_points_free(comment_points);

			gtk_signal_connect(GTK_OBJECT(data->handle),"event",(GtkSignalFunc)handle_callback,ci);

			comment_points=gnome_canvas_points_new(2);

			comment_points->coords[0]=CHARTER_X(data->t2);
			comment_points->coords[1]=CHARTER_Y(data->v2);
			comment_points->coords[2]=CHARTER_X(data->t1);
			comment_points->coords[3]=CHARTER_Y(data->v1);

			adjust_arrow_points(comment_points,data->textitem);

			data->arrow=gnome_canvas_item_new(ci->fixed_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",comment_points,
				"width_units",1.0,
                                "fill_color", color_string,
				"first_arrowhead",TRUE,
				"last_arrowhead",FALSE,
				"arrow_shape_a",8.0,
				"arrow_shape_b",8.0,
				"arrow_shape_c",4.0,
				NULL);	

			gnome_canvas_points_free(comment_points);

			gtk_signal_connect(GTK_OBJECT(data->arrow),"event",(GtkSignalFunc)arrow_callback,ci);
		}
	}
}

void create(ChartInstance *ci,xmlNodePtr config_node) {
	struct comment_data *data;
	int	i;

	data=(struct comment_data *)malloc(sizeof(struct comment_data));
	ci->instance_data=data;
	ci->instance_name=strdup("Comment");	

	if (config_node!=NULL) {
		memset(data->comment,0,1024);

		data->t1=atof(xmlGetProp(config_node,"t1"));
		data->v1=atof(xmlGetProp(config_node,"v1"));
		data->t2=atof(xmlGetProp(config_node,"t2"));
		data->v2=atof(xmlGetProp(config_node,"v2"));
		data->day=atoi(xmlGetProp(config_node,"day"));
		data->month=atoi(xmlGetProp(config_node,"month"));
		data->year=atoi(xmlGetProp(config_node,"year"));
		data->arrowenable=atoi(xmlGetProp(config_node,"arrow"));
//		sprintf(data->comment,"%s",xmlGetProp(config_node,"text"));		
		sprintf(data->comment,"%s",xmlNodeGetContent(config_node));		
		i=1;
		while (ci->quote_vector[i].year!=data->year && i<ci->quote_count) i++;
		while (ci->quote_vector[i].month!=data->month && i<ci->quote_count) i++;
		while (ci->quote_vector[i].day!=data->day && i<ci->quote_count) i++;
		data->time_offset=i-1;
		data->t1+=data->time_offset;
		data->t2+=data->time_offset;
//		data->arrowenable=pdata->arrowenable;
	} else {
		data->t1=CHARTER_RX(-100);
		data->v1=CHARTER_RY(-100);
		data->t2=CHARTER_RX(-200);
		data->v2=CHARTER_RY(-200);
		memset(data->comment,0,1024);
		sprintf(data->comment,"%s","Your comment here");
		data->day=ci->quote_vector[1].day;
		data->month=ci->quote_vector[1].month;
		data->year=ci->quote_vector[1].year;
		data->time_offset=0;
		data->arrowenable=TRUE;
	}	

	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/comment");
	
	update_xml_node(ci);
	create_comment(ci);
}

/*
void *get_pdata(void *_data,int *len) {
	struct comment_data *data=(struct comment_data *)_data;

	struct comment_persistent_data *pdata=(struct comment_persistent_data *)malloc(sizeof(struct comment_persistent_data));
	
	pdata->t1=data->t1-data->time_offset;
	pdata->t2=data->t2-data->time_offset;
	pdata->v1=data->v1;
	pdata->v2=data->v2;
	strncpy(pdata->comment,data->comment,1023);
	pdata->day=data->day;
	pdata->month=data->month;
	pdata->year=data->year;
	pdata->arrow=data->arrowenable;
	
	*len=sizeof(*pdata);
	
	return pdata;
}
*/
void on_comment_propertybox_apply (GnomePropertyBox *property_box, gint page_num) {
	char	*com;
	struct comment_data *data=(struct comment_data *)_global_ci->instance_data;

	if (page_num != -1) return;

	if (GTK_TOGGLE_BUTTON(arrowbutton)->active) {
		data->arrowenable=TRUE;
	} else {
		data->arrowenable=FALSE;
	}

	gtk_object_destroy(GTK_OBJECT(_global_ci->fixed_group));

	com=gtk_editable_get_chars(GTK_EDITABLE(commenttext),0,-1);
	strncpy(data->comment,com,1023);
	g_free(com);

	update_xml_node(_global_ci);
	create_comment(_global_ci);

//	g_print("Font: %s\n",data->fontname);	
//	g_print("Colors: %i %i %i %i\n",data->r,data->g,data->b,data->a);
}

void set_properties(ChartInstance *ci) {
	GnomePropertyBox *pbox;
	GladeXML *xxml;

	struct comment_data *data=(struct comment_data *)ci->instance_data;
	
	xxml=glade_xml_new(COMMENTGLADE_FILE,"CommentPropertyBox");
      	glade_xml_signal_autoconnect(xxml);

	pbox=(GnomePropertyBox *)glade_xml_get_widget(xxml,"CommentPropertyBox");
	commenttext=(GtkText *)glade_xml_get_widget(xxml,"commenttext");
	arrowbutton=(GtkWidget *)glade_xml_get_widget(xxml,"comment_arrow_button");

	gtk_text_set_point(commenttext,0);
	gtk_text_forward_delete(commenttext,gtk_text_get_length(commenttext));
	gtk_text_insert(commenttext,NULL,NULL,NULL,data->comment,-1);

	//Hack away the "apply"  button
	gtk_widget_hide(GNOME_PROPERTY_BOX(pbox)->apply_button);

	gnome_property_box_changed(pbox);	//Necessary, or the callback won't be called

	if (data->arrowenable) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(arrowbutton),TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(arrowbutton),FALSE);
		
	_global_ci=ci;
	gnome_dialog_run(GNOME_DIALOG(pbox));
	_global_ci=NULL;
}

void update(ChartInstance *ci) {
	struct comment_data *data=(struct comment_data *)ci->instance_data;

	if (ci->quote_count>1) {
		if (data->arrowenable) {
			gtk_object_destroy(GTK_OBJECT(data->arrow));
			gtk_object_destroy(GTK_OBJECT(data->handle));
		}
		gtk_object_destroy(GTK_OBJECT(data->textitem));
		gtk_object_destroy(GTK_OBJECT(ci->fixed_group));

		create_comment(ci);
	}
}

void redraw(ChartInstance *ci) {
	struct comment_data *data=(struct comment_data *)ci->instance_data;

	if (ci->quote_count>1) {
		if (data->arrowenable) {
			gtk_object_destroy(GTK_OBJECT(data->arrow));
			gtk_object_destroy(GTK_OBJECT(data->handle));
		}
		gtk_object_destroy(GTK_OBJECT(data->textitem));
		gtk_object_destroy(GTK_OBJECT(ci->fixed_group));

		create_comment(ci);
	}
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
