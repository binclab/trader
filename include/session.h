#ifndef SESSION_H
#define SESSION_H

#include "window.h"
#include "database.h"

void create_chart(BincWindow *window, guint last);

void update_candle(BincWindow *window, JsonObject *object);

void on_message(SoupWebsocketConnection *connection, SoupWebsocketDataType type, GBytes *message, BincWindow *window);

void on_closed(SoupWebsocketConnection *connection, BincWindow *window);

void on_websocket_connected(SoupSession *session, GAsyncResult *result, BincWindow *window);

void *setup_soup_session(void *data);

void resume_candle(BincWindow *window);

gchar *request_active_symbols();

gchar *request_subscription(gchar *symbol);

gchar *request_tick_history(gchar *symbol, int size);

gchar *request_time();

Tick *get_tick(gchar *data, gssize size);
//Candle **get_candle_history(JsonArray *list, guint length, BincWindow *window);

gchar *request_ping();

#endif // SESSION_H