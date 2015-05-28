#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include "about.h"

/*
 *	The user clicked "ok" in the about dialog. Note that this just
 *	hides the dialog because gnome_dialog_close_hides has been called
 *	in Charter.c for this dialog.
 *
 *	This is actually not needed.
 */

void on_aboutok_clicked(GtkWidget *widget, gpointer user_data) {
//	g_message("About_OK clicked\n");
}

/*
 *	This shows the about dialog. It is called when the user
 *	selects "about" from the menu.
 */

void on_about1_activate(GtkWidget *widget, gpointer user_data) {

//	g_message("Showing About Dialog...\n");
	gtk_widget_show(GTK_WIDGET(about_box));
}

/*
 *	This happens when the user clicks the close button in the window-
 *	manager decorations. I don't think this is needed anymore.
 */

gint on_charter_about_delete_event(GtkWidget *widget, gpointer user_data) {

	gtk_widget_hide(widget);
	return TRUE;
}
