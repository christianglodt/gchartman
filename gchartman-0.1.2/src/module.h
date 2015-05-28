#ifndef __MODULE_H__
#define __MODULE_H__

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <libxml/tree.h>

/*
 * These are average values
 * 1 Year = 365.25 days
 * 1 Month = 1 Year/12
 */

#define SECONDS_PER_YEAR 31557600
#define SECONDS_PER_MONTH 2629800
#define SECONDS_PER_WEEK SECONDS_PER_MONTH/4
#define SECONDS_PER_DAY 86400
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60

#define CHARTER_X(a) (-1.0*(a)*pix_per_day)
#define CHARTER_Y(a) (-1.0*(a)*pix_per_eur)
#define CHARTER_RX(a) (-1.0*(a)/pix_per_day)
#define CHARTER_RY(a) (-1.0*(a)/pix_per_eur)

GnomeCanvas	*chart_canvas;
int	quote_count_valid;
int	quote_count_invalid;

double	pix_per_day;
double	pix_per_eur;

typedef struct {
	double	value;
//	char	name[100];		//Some Text
	int	day,month,year,wday;
	int	valid;
	int	interpolated;
	time_t	timestamp;
} Quote;

typedef struct _ChartCap ChartCap;

struct _ChartCap {
	int	in_toolbar;		// toolbar button flag
	int	default_create;		// create as default when not loading a configuration
	int	deleteable;		// can be deleted
	int	new_area;		// needs seperate drawing space below main chart
	int	parent;			// can be a parent for other items
	int	child;			// can be a child (false == always child of root)
	char	*name;
	char	*type;
	char	*icon;
};

typedef struct _ChartInstance ChartInstance;

typedef struct {	
	void*	handle;		// the module handle
	void	(*create)(ChartInstance *ci, xmlNodePtr config_node);
	void	(*update)(ChartInstance *ci);
	void	(*redraw)(ChartInstance *ci);
	void	(*free_object) (ChartInstance *ci);
	void	(*set_properties) (ChartInstance *ci);
        ChartCap* (*get_caps) (void);
	ChartCap* caps;
} ChartType;

struct _ChartInstance {
	// public between module & gChartman:
	Quote	*quote_vector;		// Array of quotes
	int	quote_count;		// Number of quotes in quote_vector
	Quote	*new_quote_vector;	// Copy of quote_vector to be modified by module
	int	new_quote_count;	// Number of quotes in new_quote_vector
	double	y_offset;		// Module must set this to 0.0 if it needs redrawing after change of properties

	// maintained up-to-date by module:
	GnomeCanvasGroup *move_group;	// GnomeCanvasGroup of stuff that can be moved
	GnomeCanvasGroup *fixed_group;	// GnomeCanvasGroup of stuff that must not be moved
	char	*instance_name;		// String displayed in the chart_structure_tree
	xmlNodePtr xml_node;		// XML node containing the current module configuration

	// initialised by gChartman:
	ChartCap caps;			// The module capabilities
	ChartType *type;		// Shortcut to ChartType
	GtkCTreeNode *tree_node;	// The ctree node in the tree besides the chart (used for highlighting on mouse-over)
	ChartInstance *parent;		// The parent ChartInstance, if any

	// private to module:
	void	*instance_data;		// anonymous private data
	
	// temporary & private to gChartman:
	xmlNodePtr save_node;	
};


#endif
