#ifndef DATABASE_H
#define DATABASE_H

#include "session.h"

//static void create_symbol_database(sqlite3 *database);

Symbol *restore_last_instrument(DataObject *object);

void *save_history(void *data);
int restore_candles(CandleListModel *model);
void update_active_symbols(DataObject *dataobject, JsonArray *list);
void set_default_instrument(char *home);

#endif // DATABASE_H