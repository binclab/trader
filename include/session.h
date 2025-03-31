#ifndef SESSION_H
#define SESSION_H

#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include <libsecret/secret.h>
#include "database.h"

void setup_webview(GObject *task);
void present_actual_child(GObject *task);
void save_token_attributes(GTask *task, gpointer source, gpointer userdata, GCancellable *unused);
void load_changed(WebKitWebView *webview, WebKitLoadEvent event, gpointer userdata);
void connected(GObject *source, GAsyncResult *result, gpointer userdata);
void create_chart(GTask *task, gpointer source, gpointer userdata, GCancellable *unused);
void update_candle(GObject *object, JsonObject *item);
gchar *request_last_candle(const gchar *symbol);

const SecretSchema *get_token_schema(void);

#endif // SESSION_H