#ifndef DATABASE_H
#define DATABASE_H

#include "session.h"

//static void create_symbol_database(sqlite3 *database);

void restore_last_instrument(BincData *bincdata);

void save_history(GTask *task, gpointer source, gpointer data, GCancellable *unused);
int restore_candles(BincData *model);
void update_active_symbols(BincData *dataobject, JsonArray *list);
gboolean get_token_attributes(BincData *bincdata);
void save_token_attributes(GTask *task, gpointer source, gpointer data, GCancellable *unused);
void populate_favourite_symbols(GTask *task, gpointer source, gpointer user_data, GCancellable *unused);
void set_default_instrument(gchar *home, const gchar *symbol, const gchar *instrument);
void setup_market_navigation(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data);
void bind_market_navigation(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data);
void unbind_market_navigation(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data);
void get_new_candles(GTask *task, gpointer source, gpointer data, GCancellable *unused);

#endif // DATABASE_H