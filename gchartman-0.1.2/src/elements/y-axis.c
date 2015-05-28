#include <math.h>
#include "../module.h"

static double	min_val;
static double	max_val;

ChartCap *get_caps() {
	ChartCap *caps;
	
	caps=(ChartCap *)g_malloc(sizeof(ChartCap));
	caps->default_create=TRUE;
	caps->deleteable=FALSE;
	caps->in_toolbar=FALSE;
	caps->new_area=FALSE;
	caps->parent=FALSE;
	caps->child=FALSE;
	caps->name=strdup("Y axis");
	caps->type=strdup("gchartman/y-axis");
	caps->icon=NULL;
	return caps;
}

void free_object(ChartInstance *ci) {
	gtk_object_destroy(GTK_OBJECT(ci->fixed_group));
}

void create_yaxis(ChartInstance *ci) {
	double	height;
	double	eur_per_unit;
	double	value;
	int	i;
	int	num_units;
	char	value_string[12];
	int	cont;
	int	num_xunits;
	int	set;
	
	GnomeCanvasPoints *y_line_points;
	GnomeCanvasItem *y_line_item;
	GnomeCanvasItem *y_temp_item;
	GnomeCanvasPoints *y_temp_points;

	max_val=ci->quote_vector[1].value;
	min_val=max_val;

	for (i=1;i<ci->quote_count+1;i++) {
		if (ci->quote_vector[i].value < min_val) min_val=ci->quote_vector[i].value; //Find minimum
		if (ci->quote_vector[i].value > max_val) max_val=ci->quote_vector[i].value; //Find maximum
	}

	height=max_val*pix_per_eur;

	ci->fixed_group=(GnomeCanvasGroup *)gnome_canvas_item_new(gnome_canvas_root(chart_canvas),
					GNOME_TYPE_CANVAS_GROUP,
					"x",0.0,
					"y",0.0,
					NULL);

	gnome_canvas_item_lower_to_bottom(GNOME_CANVAS_ITEM(ci->fixed_group));

	num_xunits=ci->quote_count;

	num_units=15; // 15 Ticks please
	num_units+=2;

	eur_per_unit=pow(10,rint(log10(height)));
	
	while ((height/eur_per_unit)<(height/50)) {	// 50 Pixels font height
		eur_per_unit=rint(eur_per_unit/2);
	}
//	printf("eur_per_unit: %f pix_per_eur: %f\n",eur_per_unit,pix_per_eur);
	
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

//	printf("eur_per_unit: %f\n",eur_per_unit);
	
	value=0.0;

	i=0;
	cont=FALSE;
	while (cont==FALSE) {
		// Create ticks for the Y axis here
		y_temp_points=gnome_canvas_points_new(2);
		y_temp_points->coords[0]=0.0;
		y_temp_points->coords[1]=i*CHARTER_Y(eur_per_unit);
		y_temp_points->coords[2]=5.0;
		y_temp_points->coords[3]=i*CHARTER_Y(eur_per_unit);
		y_temp_item=gnome_canvas_item_new(ci->fixed_group,
					GNOME_TYPE_CANVAS_LINE,
					"points",y_temp_points,
					"width_units",0.1,
	                                "fill_color", "black",
					NULL);

		gnome_canvas_item_lower_to_bottom(y_temp_item);

		gnome_canvas_points_free(y_temp_points);

		// Create ticks for the Y axis here
		y_temp_points=gnome_canvas_points_new(2);
		y_temp_points->coords[0]=CHARTER_X(ci->quote_count-1);
		y_temp_points->coords[1]=i*CHARTER_Y(eur_per_unit);
		y_temp_points->coords[2]=0.0;
		y_temp_points->coords[3]=i*CHARTER_Y(eur_per_unit);
		y_temp_item=gnome_canvas_item_new(ci->fixed_group,
					GNOME_TYPE_CANVAS_LINE,
					"points",y_temp_points,
					"width_units",0.1,
	                                "fill_color", "#A8A8A8",
					NULL);

		gnome_canvas_item_lower_to_bottom(y_temp_item);

		gnome_canvas_points_free(y_temp_points);

		sprintf(value_string,"%.2f",value);
	
		y_temp_item=gnome_canvas_item_new (ci->fixed_group,
                               GNOME_TYPE_CANVAS_TEXT,
                               "text",value_string,
                               "x", 5.0+1.0,
                               "y", i*CHARTER_Y(eur_per_unit),
                               "font", "-b&h-lucida-medium-r-normal-*-*-100-*-*-p-*-iso8859-1",
                               "anchor", GTK_ANCHOR_SW,
                               "fill_color", "black",
                               NULL);

		gnome_canvas_item_lower_to_bottom(y_temp_item);

		value+=eur_per_unit;
		if ((i*eur_per_unit)>max_val) cont=TRUE;
		i++;
	}

	y_line_points=gnome_canvas_points_new(2);
	y_line_points->coords[0]=0.0;
	y_line_points->coords[1]=CHARTER_Y((i-1)*eur_per_unit);
	y_line_points->coords[2]=0.0;
	y_line_points->coords[3]=0.0;

	y_line_item=gnome_canvas_item_new(ci->fixed_group,
				GNOME_TYPE_CANVAS_LINE,
				"points",y_line_points,
				"width_units",1.0,
                                "fill_color", "black",
				NULL);	

	gnome_canvas_item_lower_to_bottom(y_line_item);

	gnome_canvas_points_free(y_line_points);
}

void update(ChartInstance *ci) {

	gtk_object_destroy(GTK_OBJECT(ci->fixed_group));

	create_yaxis(ci);
}

void redraw(ChartInstance *ci) {

	gtk_object_destroy(GTK_OBJECT(ci->fixed_group));

	create_yaxis(ci);
}

void create(ChartInstance *ci,xmlNodePtr config_node) {
	
	ci->xml_node=xmlNewNode(NULL,"item");
	xmlSetProp(ci->xml_node,"type","gchartman/y-axis");
	ci->instance_name=strdup("Y axis");
	ci->move_group=NULL;
	create_yaxis(ci);
}
