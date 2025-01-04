#ifndef SESSION_H
#define SESSION_H

#include "window.h"
#include "database.h"

void create_chart(BincWindow *window);

void update_candle(BincWindow *window, JsonObject *object);

void *setup_soup_session(void *argument);

void resume_candle(BincWindow *window);

gchar *request_active_symbols();

gchar *request_subscription(gchar *symbol);

gchar *request_tick_history(gchar *symbol, int size);

gchar *request_time();

Tick *get_tick(gchar *data, gssize size);
//Candle **get_candle_history(JsonArray *list, guint length, BincWindow *window);

gchar *request_ping();

#endif // SESSION_H