#include <math.h>
#include <libxml/tree.h>
#include <libxml/entities.h>
#include "../module.h"

#define	XAXISGLADE_FILE PLUGINDIR"/x-axisprop.glade"

struct xaxis_data {
	guint8	r,g,b,a;
	gchar *fontname;
	Quote	*quote_vector;
	int	quote_count;
	int	display_grid;
	int	display_subgrid;
};

// This structure is saved in the database.
// You may not have pointers in here, only a solid block of data.
/*
struct persistent_data {
	guint8	r,g,b,a;
	char	fontname[200];
	int	display_grid;
	int	display_subgrid;
};
*/
static void create_xaxis(ChartInstance *ci);
static void x_update_xml_node(ChartInstance *ci);

static GnomeColorPicker *cpick;
static GnomeFontPicker *fpick;
static GtkWidget *grid_checkbutton;
static GtkWidget *subgrid_checkbutton;

static ChartInstance *_prop_ci;	//Only for use in PropertyBox handlers !!!

void on_xaxis_propertybox_apply (GnomePropertyBox *property_box, gint page_num) {
	struct xaxis_data *data=(struct xaxis_data *)_prop_ci->instance_data;

	if (page_num != -1) return;

	gnome_color_picker_get_i8(cpick,&data->r,&data->g,&data->b,&data->a);
	data->fontname=strdup(gnome_font_picker_get_font_name(fpick));
	data->display_grid=GTK_TOGGLE_BUTTON(grid_checkbutton)->active;
	data->display_subgrid=GTK_TOGGLE_BUTTON(subgrid_checkbutton)->active;

	gtk_object_destroy(GTK_OBJECT(_prop_ci->fixed_group));
	gtk_object_destroy(GTK_OBJECT(_prop_ci->move_group));

	create_xaxis(_prop_ci);
	x_update_xml_node(_prop_ci);

//	g_print("Font: %s\n",data->fontname);	
//	g_print("Colors: %i %i %i %i\n",data->r,data->g,data->b,data->a);
}

void set_properties(ChartInstance *ci) {
	struct xaxis_data *data=(struct xaxis_data *)ci->instance_data;
	GnomePropertyBox *pbox;
	GladeXML *xxml;

	_prop_ci=ci;
	
	xxml=glade_xml_new(XAXISGLADE_FILE,"XPropertyBox");
      	glade_xml_signal_autoconnect(xxml);

	pbox=(GnomePropertyBox *)glade_xml_get_widget(xxml,"XPropertyBox");
	cpick=(GnomeColorPicker *)glade_xml_get_widget(xxml,"colorpicker1");
	fpick=(GnomeFontPicker *)glade_xml_get_widget(xxml,"fontpicker1");
	grid_checkbutton=(GtkWidget *)glade_xml_get_widget(xxml,"grid_checkbutton");
	subgrid_checkbutton=(GtkWidget *)glade_xml_get_widget(xxml,"subgrid_checkbutton");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid_checkbutton),data->display_grid);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(subgrid_checkbutton),data->display_subgrid);

	//Hack away the "apply"  button
	gtk_widget_hide(GNOME_PROPERTY_BOX(pbox)->apply_button);

	gnome_property_box_changed(pbox);	//Necessary, or the callback won't be called

	gnome_color_picker_set_i8(cpick,data->r,data->g,data->b,data->a);
	gnome_font_picker_set_font_name(fpick,data->fontname);
	
	gnome_dialog_run(GNOME_DIALOG(pbox));
	
//	gtk_widget_destroy(pbox);
//	gtk_widget_destroy(xxml);
}

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=TRUE;
	caps->deleteable=FALSE;
	caps->in_toolbar=FALSE;
	caps->new_area=FALSE;
	caps->parent=FALSE;
	caps->child=FALSE;
	caps->name=strdup("X axis");
	caps->type=strdup("gchartman/x-axis");
	caps->icon=NULL;
	return caps;
}

void free_object(ChartInstance *ci) {
	struct xaxis_data *data=(struct xaxis_data *)ci->instance_data;	// Cast is necessary...

	gtk_object_destroy(GTK_OBJECT(ci->move_group));
	gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
	g_free(data);
	g_free(ci->instance_name);
	xmlFreeNode(ci->xml_node);
}

static GnomeCanvasItem *add_tick(ChartInstance *ci,int number,int text,double tick_unit,char *color_string,double gridheight) {
	struct xaxis_data *data=(struct xaxis_data *)ci->instance_data;
	GnomeCanvasPoints *temp_points;
	GnomeCanvasItem *temp_item;
	GnomeCanvasItem *temp_item2;
	char	time_string[12];

	// Create upper grid here
	temp_points=gnome_canvas_points_new(2);

	temp_points->coords[0]=(number-1)*CHARTER_X(tick_unit);

	if (data->display_grid==FALSE) temp_points->coords[1]=0.0;
	else temp_points->coords[1]=gridheight;

	temp_points->coords[2]=(number-1)*CHARTER_X(tick_unit);
	temp_points->coords[3]=0;
		
	temp_item=gnome_canvas_item_new(ci->fixed_group,
		GNOME_TYPE_CANVAS_LINE,
		"points",temp_points,
		"width_units",0.1,
		"fill_color", "#A8A8A8",
		NULL);

	gnome_canvas_item_lower_to_bottom(temp_item);

	gnome_canvas_points_free(temp_points);		

	// Create lower part of ticks here
	temp_points=gnome_canvas_points_new(2);

	temp_points->coords[0]=(number-1)*CHARTER_X(tick_unit);

	temp_points->coords[1]=0.0;

	temp_points->coords[2]=(number-1)*CHARTER_X(tick_unit);
	temp_points->coords[3]=5.0;
		
	temp_item=gnome_canvas_item_new(ci->move_group,
		GNOME_TYPE_CANVAS_LINE,
		"points",temp_points,
		"width_units",0.1,
		"fill_color", color_string,
		NULL);

	gnome_canvas_item_lower_to_bottom(temp_item);

	gnome_canvas_points_free(temp_points);		

	if (text==TRUE) {
		sprintf(time_string,"%i.%i",ci->quote_vector[number].day,ci->quote_vector[number].month+1);

		temp_item2=gnome_canvas_item_new (ci->move_group,
			GNOME_TYPE_CANVAS_TEXT,
			"text",time_string,
			"x", (number-1)*CHARTER_X(tick_unit),
			"y", 5.0+1.0,
			"font", data->fontname,
			"anchor", GTK_ANCHOR_N,
			"fill_color", color_string,
			NULL);

		gnome_canvas_item_lower_to_bottom(temp_item2);

	}

	return temp_item;
}

static void create_xaxis(ChartInstance *ci) {
	struct xaxis_data *data=(struct xaxis_data *)ci->instance_data;
	double	width,max_val,height,eur_per_unit,gridheight,subgridheight;
	int	i,cont;
	int	num_units;
	double	tick_unit;
	double	subtick_unit;
	int	num_subticks;
	int	last_month;
	
	time_t	x_lasttime;
	time_t	x_firsttime;
	char	color_string[8];
	char	*red_color="#FF0000";
	int	set;

	GnomeCanvasPoints *x_line_points;
	GnomeCanvasItem *temp_item;

	sprintf(color_string,"#%.2X%.2X%.2X",data->r,data->g,data->b);

	x_lasttime=ci->quote_vector[1].timestamp;
	x_firsttime=x_lasttime;

	max_val=0;	
	for (i=1;i<ci->quote_count+1;i++) {
		if (ci->quote_vector[i].timestamp < x_firsttime) {x_firsttime=ci->quote_vector[i].timestamp;}
		if (ci->quote_vector[i].timestamp > x_lasttime) {x_lasttime=ci->quote_vector[i].timestamp;}
		if (ci->quote_vector[i].value > max_val) max_val=ci->quote_vector[i].value; //Find maximum
	}

	// (kludge) Copied from y-axis.c:
	height=max_val*pix_per_eur;
	eur_per_unit=pow(10,rint(log10(height)));
	while ((height/eur_per_unit)<(height/50)) {	// 50 Pixels font height
		eur_per_unit=rint(eur_per_unit/2);
	}

	set=FALSE;

	if ((eur_per_unit/pix_per_eur)>10000000 && set==FALSE) {eur_per_unit=10000000*rint((eur_per_unit/pix_per_eur)*0.0000001); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>1000000 && set==FALSE) {eur_per_unit=1000000*rint((eur_per_unit/pix_per_eur)*0.000001); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>100000 && set==FALSE) {eur_per_unit=100000*rint((eur_per_unit/pix_per_eur)*0.00001); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>10000 && set==FALSE) {eur_per_unit=10000*rint((eur_per_unit/pix_per_eur)*0.0001); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>1000 && set==FALSE) {eur_per_unit=1000*rint((eur_per_unit/pix_per_eur)*0.001); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>100 && set==FALSE) {eur_per_unit=100*rint((eur_per_unit/pix_per_eur)*0.01); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>10 && set==FALSE) {eur_per_unit=10*rint((eur_per_unit/pix_per_eur)*0.1); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>1 && set==FALSE) {eur_per_unit=1.0*rint((eur_per_unit/pix_per_eur)*1); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>0.1 && set==FALSE) {eur_per_unit=0.1*rint((eur_per_unit/pix_per_eur)*10); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>0.01 && set==FALSE) {eur_per_unit=0.001*rint((eur_per_unit/pix_per_eur)*1000); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>0.001 && set==FALSE) {eur_per_unit=0.0001*rint((eur_per_unit/pix_per_eur)*10000); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>0.0001 && set==FALSE) {eur_per_unit=0.00001*rint((eur_per_unit/pix_per_eur)*100000); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>0.00001 && set==FALSE) {eur_per_unit=0.000001*ceil((eur_per_unit/pix_per_eur)*1000000); set=TRUE;}
	if ((eur_per_unit/pix_per_eur)>0 && set==FALSE) {eur_per_unit=0.0000001*ceil((eur_per_unit/pix_per_eur)*10000000); set=TRUE;}

	i=0;
	cont=FALSE;
	while (cont==FALSE) {
		if ((i*eur_per_unit)>max_val) cont=TRUE;
		i++;
	}

	if (data->display_grid) {
		gridheight=CHARTER_Y((i-1)*eur_per_unit);
	} else {
		gridheight=0;
	}

	if (data->display_subgrid) {
		subgridheight=gridheight;
	} else {
		subgridheight=0;
	}

	width=-1*CHARTER_X(ci->quote_count);

	ci->fixed_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
		GNOME_TYPE_CANVAS_GROUP,
		"x",0.0,
		"y",0.0,
		NULL);

	gnome_canvas_item_lower_to_bottom(GNOME_CANVAS_ITEM(ci->fixed_group));

	ci->move_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
		GNOME_TYPE_CANVAS_GROUP,
		"x",0.0,
		"y",0.0,
		NULL);

	gnome_canvas_item_lower_to_bottom(GNOME_CANVAS_ITEM(ci->move_group));

	num_units=ci->quote_count;
	subtick_unit=SECONDS_PER_MONTH;
	num_subticks=11;
	tick_unit=1;	//days
	
	x_line_points=gnome_canvas_points_new(2);
	x_line_points->coords[0]=0.0;
	x_line_points->coords[1]=0.0;
	x_line_points->coords[2]=CHARTER_X(ci->quote_count-1);
	x_line_points->coords[3]=0.0;

	temp_item=gnome_canvas_item_new(ci->move_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",x_line_points,
				"width_units",1.0,
                                "fill_color", color_string,
				NULL);	

	gnome_canvas_item_lower_to_bottom(temp_item);

	gnome_canvas_points_free(x_line_points);		

//	g_print("ppd: %f\n",pix_per_day);

	last_month=13;

	for (i=1;i<ci->quote_count+1;i++) {

		if (pix_per_day > 23) {
		  add_tick(ci,i,TRUE,tick_unit,color_string,gridheight);
		} else {
		  if (pix_per_day > 3.85) {
		    if (ci->quote_vector[i].wday==1) {
		      add_tick(ci,i,TRUE,tick_unit,color_string,gridheight);
		    } else {
			if (pix_per_day > 5.64) {
			  add_tick(ci,i,FALSE,tick_unit,color_string,subgridheight);
			}
		    }
		  } else {
		    if (pix_per_day > 1) {
		      if (ci->quote_vector[i].month!=last_month) {
		      	last_month=ci->quote_vector[i].month;
		        add_tick(ci,i,TRUE,tick_unit,color_string,gridheight);
		      }
		    }
		  }
		}
		
		// Mark days without data
		if (ci->quote_vector[i].interpolated) {
		  temp_item=add_tick(ci,i,FALSE,tick_unit,red_color,-5);
		  gnome_canvas_item_raise(temp_item,2);
		}
	}

	// add leftmost tick just to be sure
        add_tick(ci,ci->quote_count,FALSE,tick_unit,color_string,gridheight);
	
}

void update(ChartInstance *ci) {

	gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
	gtk_object_destroy(GTK_OBJECT(ci->move_group));

	create_xaxis(ci);

}

void redraw(ChartInstance *ci) {
	gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
	gtk_object_destroy(GTK_OBJECT(ci->move_group));

	create_xaxis(ci);
}

static void x_update_xml_node(ChartInstance *ci) {
	struct xaxis_data *data=(struct xaxis_data *)ci->instance_data;
	char	temp_string[500];

	sprintf(temp_string,"%i",data->r);
	xmlSetProp(ci->xml_node,"r",temp_string);
	sprintf(temp_string,"%i",data->g);
	xmlSetProp(ci->xml_node,"g",temp_string);
	sprintf(temp_string,"%i",data->b);
	xmlSetProp(ci->xml_node,"b",temp_string);
	sprintf(temp_string,"%i",data->a);
	xmlSetProp(ci->xml_node,"a",temp_string);

	sprintf(temp_string,"%i",data->display_grid);
	xmlSetProp(ci->xml_node,"display_grid",temp_string);
	sprintf(temp_string,"%i",data->display_subgrid);
	xmlSetProp(ci->xml_node,"display_subgrid",temp_string);

	xmlSetProp(ci->xml_node,"fontname",data->fontname);

}

void create(ChartInstance *ci,xmlNodePtr config_node) {
	struct xaxis_data *data=(struct xaxis_data *)ci->instance_data;

	data=(struct xaxis_data *)malloc(sizeof(struct xaxis_data));
	ci->instance_data=data;

	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/x-axis");

	if (config_node!=NULL) {

		data->r=atoi(xmlGetProp(config_node,"r"));
		data->g=atoi(xmlGetProp(config_node,"g"));
		data->b=atoi(xmlGetProp(config_node,"b"));
		data->a=atoi(xmlGetProp(config_node,"a"));
		data->display_grid=atoi(xmlGetProp(config_node,"display_grid"));
		data->display_subgrid=atoi(xmlGetProp(config_node,"display_subgrid"));

		// FIXME: Get fontname from xml node
//		data->fontname=strdup("-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1");
		data->fontname=strdup(xmlGetProp(config_node,"fontname"));
		

/*		data->r=pdata->r;
		data->g=pdata->g;
		data->b=pdata->b;
		data->a=pdata->a;
		data->fontname=strdup("-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1");
//		data->fontname=strdup(pdata->fontname);
		data->display_grid=pdata->display_grid;
		data->display_subgrid=pdata->display_subgrid;
*/	} else {
		data->r=0;
		data->g=0;
		data->b=0;
		data->a=0;
		data->fontname=strdup("-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1");
		data->display_grid=TRUE;
		data->display_subgrid=FALSE;
	}	

	create_xaxis(ci);
	x_update_xml_node(ci);
	ci->instance_name=strdup("X Axis");
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
