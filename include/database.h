#ifndef DATABASE_H
#define DATABASE_H

#include "symbols.h"
#include "session.h"

void create_symbol_database(char *home);

Symbol *restore_last_instrument(char *home);

void *save_history(void *data);
int restore_candles(CandleInfo *info);
void update_active_symbols(char *home, JsonArray *list);
void set_default_instrument(char *home);

#endif // DATABASE_H