#ifndef SESSION_H
#define SESSION_H

#include "symbols.h"
#include "database.h"

void create_chart(BincData *bincdata);

void update_candle(BincData *bincdata, JsonObject *object);

void setup_soup_session(GTask *task, gpointer source, gpointer data, GCancellable *unused);

gchar *request_active_symbols();

gchar *request_subscription(gchar *symbol);

gchar *request_tick_history(gchar *symbol, int size);

gchar *request_time();

BincTick *get_tick(gchar *data, gssize size);
//Candle **get_candle_history(JsonArray *list, guint length, BincWindow *window);

gchar *request_ping();

#endif // SESSION_H