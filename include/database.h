#ifndef DATABASE_H
#define DATABASE_H

#include "session.h"

//static void create_symbol_database(sqlite3 *database);

BincSymbol *restore_last_instrument(BincData *bincdata);

void save_history(GTask *task, gpointer source, gpointer data, GCancellable *unused);
int restore_candles(BincData *model);
void update_active_symbols(BincData *dataobject, JsonArray *list);
gboolean get_token_attributes(BincData *bincdata);
void save_token_attributes(GTask *task, gpointer source, gpointer data, GCancellable *unused);

void set_default_instrument(char *home);

#endif // DATABASE_H