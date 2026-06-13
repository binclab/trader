#ifndef SERVER_H
#define SERVER_H

#include <glib-object.h>
#include <libsoup/soup.h>
#include <sqlite3.h>

G_BEGIN_DECLS

#define APP_TYPE_CONTEXT (app_context_get_type())
G_DECLARE_FINAL_TYPE(AppContext, app_context, APP, CONTEXT, GObject)

AppContext* app_context_new(sqlite3* db, SoupSession* session);

sqlite3* app_context_get_database(AppContext* self);
SoupSession* app_context_get_session(AppContext* self);
#endif  // SERVER_H