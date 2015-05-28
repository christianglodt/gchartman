#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>
#include "gchartman.h"
#include "connectdb.h"

void on_connectdbpassword_activate(GtkWidget *widget, gpointer user_data) {
	gtk_signal_emit_by_name(GTK_OBJECT(connectdb_ok),"clicked");
}

/*
 *	The user clicked the "Disconnect" toolbar button. The code is the
 *	same as for on_disconnect_activate.
 *	In/Out:	Std. Gtk+ callback.
 */

void on_disconnect_clicked(GtkWidget *widget, gpointer user_data) {
	if (mysql!=NULL) {
		g_hash_table_destroy(name_cache);
		g_hash_table_destroy(tname_cache);
		name_cache=NULL;
		tname_cache=NULL;
		mysql_close(mysql);
		mysql=NULL;
//		g_message("Disconnected from Database.\n");
		gtk_label_set_text(database_status_label,"Not connected to a database.");
		update_tablelist(mysql);
		gtk_widget_set_sensitive(GTK_WIDGET(trading_time_button),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(disconnect_button),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(add_category_button),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(connect_button),TRUE);
	}
}

/*
 *	The user selected "Disconnect" from the main menu. The code is the
 *	same as for on_disconnect_clicked.
 *	In/Out:	Std. Gtk+ callback.
 */

void on_disconnect_activate(GtkWidget *widget, gpointer user_data) {
	if (mysql!=NULL) {
		g_hash_table_destroy(name_cache);
		g_hash_table_destroy(tname_cache);
		name_cache=NULL;
		tname_cache=NULL;
		mysql_close(mysql);
		mysql=NULL;
//		g_message("Disconnected from Database.\n");
		gtk_label_set_text(database_status_label,"Not connected to a database.");
		update_tablelist(mysql);
		gtk_widget_set_sensitive(GTK_WIDGET(trading_time_button),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(disconnect_button),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(connect_button),TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(add_category_button),FALSE);
	}
}

/*
 *	The user clicked the "Connect" button on the toolbar. This displays
 *	the "Connect"-dialog. (Same as on_DBConnect_activate).
 *	In/Out:	Std. Gtk+ callback.
 */

void on_dbconnecttoolbar_clicked(GtkWidget *widget, gpointer user_data) {
//	gtk_signal_emit_by_name(GTK_OBJECT(ConnectDBpassword),"focus_in_event");
	gtk_widget_grab_focus(GTK_WIDGET(connectdb_password));
	gnome_dialog_run(connectdb_dialog);
}

/*
 *	The user selected "Connect" from the main menu. This displays
 *	the "Connect"-dialog. (Same as on_DBConnectToolbar_clicked).
 *	In/Out:	Std. Gtk+ callback.
 */

void on_dbconnect_activate(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_run(connectdb_dialog);
}

/*
 *	This is called when the user clicks "ok" in the dialog that
 *	queries for details of the database to connect to. It tries
 *	to connect to the given database. If it fails, it asks the
 *	user if he wants to create a new database. This function
 *	initializes the global MYSQL-object.
 *	In/Out:	Std. Gtk+ callback.
 */

void on_connectdbok_clicked(GtkWidget *widget, gpointer user_data) {
	gchar	*host;
	gchar	*user;
	gchar	*password;
	gchar	*dbname;
	gchar	dbquery[200];
	gchar	dbstatus[1000];
	int	broken,found;
	xmlNodePtr node;

	host=gtk_entry_get_text((GtkEntry *)gnome_entry_gtk_entry(connectdb_host));
	user=gtk_entry_get_text((GtkEntry *)gnome_entry_gtk_entry(connectdb_user));
	password=gtk_entry_get_text(connectdb_password);
	dbname=gtk_entry_get_text((GtkEntry *)gnome_entry_gtk_entry(connectdb_database));

	broken=FALSE;

	mysql=mysql_init(NULL);
	if (!mysql_real_connect(mysql,host,user,password,NULL,0,NULL,0)) {
		error_dialog("Could not connect to Database");
		fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(mysql));
		mysql_close(mysql);
		mysql=NULL;
	} else {
		sprintf(dbquery,"use %s",dbname);
		mysql_query(mysql,dbquery);
	
		if (mysql_errno(mysql)!=0) {
			switch (ok_cancel_dialog("The database does not exist.\nCreate a new one?")) {
				case 0:
					g_message("Creating new Database %s.\n",dbname);
					sprintf(dbquery,"create database %s",dbname);
					if (mysql_query(mysql,dbquery)!=0) {
						fprintf(stderr, "Failed to create database: Error: %s\n", mysql_error(mysql));
					}
					sprintf(dbquery,"use %s",dbname);
					if (mysql_query(mysql,dbquery)!=0) {
						fprintf(stderr, "Failed to use database: Error: %s\n", mysql_error(mysql));
					}
					break;
				case 1:
					broken=TRUE;
					break;
				default:
					broken=TRUE;
					break;
			}
		}
		
		if (broken==FALSE) {		
			sprintf(dbquery,"create table _TableNames (Name Text,TName Text,Comment Text)");
			mysql_query(mysql,dbquery);
			name_cache=g_hash_table_new(g_str_hash,g_str_equal);
			tname_cache=g_hash_table_new(g_str_hash,g_str_equal);
		
			sprintf(dbstatus,"Database: %s\nuser:     %s\nhost:     %s",dbname,user,host);
			gtk_label_set_text(database_status_label,dbstatus);
		// -------------------------------
			found=FALSE;
			node=global_configs_doc->children;
			while (node) {
				if (strcmp(node->name,"configurations")==0) {
					node=node->children;
					break;
				}
				node=node->next;
			}

			if (node) {
				while (node) {
					if (strcmp(node->name,"dbconfig")==0) {
						if (strcmp(xmlGetProp(node,"dbname"),dbname)==0) {
							found=TRUE;
							db_global_configs_node=node;
							break;
						}
					}
					node=node->next;
				}
			}
			if (!found) {
				db_global_configs_node=xmlNewNode(NULL,"dbconfig");
				xmlSetProp(db_global_configs_node,"dbname",dbname);
				xmlAddChild(global_configs_doc->children,db_global_configs_node);
			}
		// ---------------------------------
			found=FALSE;
			node=local_configs_doc->children;
			while (node) {
				if (strcmp(node->name,"configurations")==0) {
					node=node->children;
					break;
				}
				node=node->next;
			}

			if (node) {
				while (node) {
					if (strcmp(node->name,"dbconfig")==0) {
						if (strcmp(xmlGetProp(node,"dbname"),dbname)==0) {
							found=TRUE;
							db_local_configs_node=node;
							break;
						}
					}
					node=node->next;
				}
			}
			if (!found) {
				db_local_configs_node=xmlNewNode(NULL,"dbconfig");
				xmlSetProp(db_local_configs_node,"dbname",xmlEncodeEntitiesReentrant(local_configs_doc,dbname));
				xmlAddChild(local_configs_doc->children,db_local_configs_node);
			} else {
				gtk_widget_set_sensitive(GTK_WIDGET(connect_button),FALSE);
				gtk_widget_set_sensitive(GTK_WIDGET(disconnect_button),TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(add_category_button),TRUE);
			}
		// ---------------------------------
			gnome_dialog_close(connectdb_dialog);
			update_tablelist(mysql);

		}
	}
}

/*
 *	This is called when the user clicks cancel in the dialog which asks
 *	details about the database to connect to.
 *	In/Out:	Std. Gtk+ callback.
 */

void on_connectdbcancel_clicked(GtkWidget *widget, gpointer user_data) {
	gnome_dialog_close(connectdb_dialog);
}
