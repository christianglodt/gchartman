#include <mysql/mysql.h>

/*
 *	This is the mysql object that is used through-out the
 *	whole program.
 */

MYSQL *mysql;

/*
 *	Global widgets.
 */

GtkWidget *connectdb_ok;

GnomeDialog *connectdb_dialog;
GnomeEntry *connectdb_host;
GnomeEntry *connectdb_user;
GtkEntry *connectdb_password;
GnomeEntry *connectdb_database;
