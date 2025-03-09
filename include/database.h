#ifndef DATABASE_H
#define DATABASE_H

#include <json-glib/json-glib.h>
#include <sqlite3.h>

gint get_saved_candles(GObject *task, const gchar *symbol, const gchar *timeframe);
void setup_symbol(GObject *task, sqlite3 *database);
void save_history(GTask *handler, gpointer source, gpointer userdata, GCancellable *unused);
void update_active_symbols(GObject *task, JsonArray *list);
void setup_user_interface(GObject *task);
#endif // DATABASE_H