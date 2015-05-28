#define	SEL_NAME	0
#define	SEL_VALUE	1
#define	SEL_VOLUME	2
#define	SEL_MINIMUM	3
#define	SEL_MAXIMUM	4
#define	SEL_TOPBOTTOM	5
#define NUM_SELS	6

#define X_P_BOUND	280
#define X_N_BOUND	0
#define Y_P_BOUND	325
#define Y_N_BOUND	0

typedef struct {
	int	disabled;
	double	c1,c2;
} VtSelection;

typedef struct VtPage {
	struct VtPage	*next;			// Pointer to next Page
	struct VtPage	*previous;
	char	category[50];
	int	row_number;	// This ID is different for Pages, not for Subpages
	int	page_number;
	int	sub_page_number;		// should be -1 for single pages
	char	filename[50];
	VtSelection page_selections[NUM_SELS];
	char	page_text[26][41];	//don't forget the linefeed and some zeros
	time_t	time;
} VtPage;

GnomeAppBar	*vtwin_appbar;

GtkCList	*unknown_dialog_existing_stocks_clist;

GnomeDialog	*unknown_dialog;
GnomeEntry	*unknown_stock_name;
GtkEntry	*unknown_stock_name_entry;
GtkEntry	*unknown_stock_tname;
GtkCheckButton	*tname_ok_checkbutton;
GnomeEntry	*unknown_stock_value;
GnomeEntry	*unknown_stock_minimum;
GnomeEntry	*unknown_stock_maximum;
GnomeEntry	*unknown_stock_volume;

GnomeCanvasPoints *points1[NUM_SELS];
GnomeCanvasPoints *points2[NUM_SELS];
GnomeCanvasItem	*lines1[NUM_SELS];
GnomeCanvasItem	*lines2[NUM_SELS];
GnomeCanvasItem *rect[NUM_SELS];	//SEL_TOPBOTTOM has no Rectangle

VtPage	vtpage_header;
VtPage	*vtpages;
VtPage	*vttemp;

double xsize;
double ysize;
double xstep;
double ystep;

gint single_page_num;
gint multi_page_num;
gint multi_page_num_start;
gint multi_page_num_end;
gint selected_row;

GtkLabel *vtpage_displayed_label;

GtkButton *vt_up;
GtkButton *vt_down;
GtkButton *vt_left;
GtkButton *vt_right;
GtkButton *vt_taller;
GtkButton *vt_smaller;

GtkWidget *vt_box;
GtkWidget *desired_vtpage;
GnomeCanvas *vt_select_area;
GnomeCanvasItem *vtsa_text;
GtkCList *page_list;
GnomeDialog *add_page_dialog;
GtkWidget *vt_timeout_window;
GtkProgress *vt_progress_bar;
GtkNotebook *add_page_notebook;
GtkSpinButton *single_page_num_spin;
GtkSpinButton *multi_page_num_spin;
GtkSpinButton *multi_page_start_spin;
GtkSpinButton *multi_page_end_spin;
GtkToggleButton *disable_button;

GtkCombo *select_category_combo;

extern void on_get_vt_quotes1_activate(GtkWidget *widget, gpointer user_data);
extern void on_getvtbutton_clicked(GtkWidget *widget, gpointer user_data);
extern void update_selection();
char *escape_name(char *name);
