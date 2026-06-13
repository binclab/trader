#include "server.h"

#include <errno.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <sqlite3.h>
#include <string.h>
#include <unistd.h>

#include "glib.h"

struct _AppContext
{
    GObject parent_instance;

    sqlite3* database;
    SoupSession* session;
};

G_DEFINE_TYPE(AppContext, app_context, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_DATABASE,
    PROP_SESSION,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES];

static void app_context_set_property(GObject* object, guint prop_id, const GValue* value,
                                     GParamSpec* pspec)
{
    AppContext* self = APP_CONTEXT(object);

    switch (prop_id)
    {
        case PROP_DATABASE:
            self->database = g_value_get_pointer(value);
            break;

        case PROP_SESSION:
            self->session = g_value_get_object(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void app_context_get_property(GObject* object, guint prop_id, GValue* value,
                                     GParamSpec* pspec)
{
    AppContext* self = APP_CONTEXT(object);

    switch (prop_id)
    {
        case PROP_DATABASE:
            g_value_set_pointer(value, self->database);
            break;

        case PROP_SESSION:
            g_value_set_object(value, self->session);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void app_context_class_init(AppContextClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = app_context_set_property;
    object_class->get_property = app_context_get_property;

    obj_properties[PROP_DATABASE] =
        g_param_spec_pointer("database", "Database", "SQLite database handle",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    obj_properties[PROP_SESSION] =
        g_param_spec_object("session", "Session", "SoupSession instance", SOUP_TYPE_SESSION,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void app_context_init(AppContext* self) {}

AppContext* app_context_new(sqlite3* db, SoupSession* session)
{
    return g_object_new(APP_TYPE_CONTEXT, "database", db, "session", session, NULL);
}

sqlite3* app_context_get_database(AppContext* self) { return self->database; }

SoupSession* app_context_get_session(AppContext* self) { return self->session; }

static void on_message(SoupWebsocketConnection* connection, SoupWebsocketDataType type,
                       GBytes* message, gpointer user_data);
static void connection_closed(SoupWebsocketConnection* connection, gpointer user_data);

static JsonObject* perform_token_exchange(SoupSession* session, const gchar* code,
                                          const gchar* code_verifier, const gchar* client_id,
                                          GError** error)
{
    g_printerr("perform_token_exchange: code=%s verifier=%s\n", code, code_verifier);
    SoupMessage* msg = soup_message_new("POST", "https://auth.deriv.com/oauth2/token");
    const char* redirect = "https://trader.binclab.com/index";
    gchar* body;

    if (client_id && *client_id)
    {
        body = g_strdup_printf(
            "grant_type=authorization_code&client_id=%s&code_verifier=%s&code=%s&redirect_uri=%s",
            client_id, code_verifier, code, redirect);
    }
    else
    {
        body = g_strdup_printf(
            "grant_type=authorization_code&code_verifier=%s&code=%s&redirect_uri=%s", code_verifier,
            code, redirect);
    }

    GBytes* bytes =
        g_bytes_new_take(body, strlen(body));  // Takes ownership of body, eliminating 1 copy
    soup_message_set_request_body_from_bytes(msg, "application/x-www-form-urlencoded", bytes);
    g_bytes_unref(bytes);

    GError* err = NULL;
    GBytes* resp = soup_session_send_and_read(session, msg, NULL, &err);
    guint status = soup_message_get_status(msg);

    if (resp == NULL)
    {
        if (err)
            g_propagate_error(error, err);
        else
            g_set_error(error, g_quark_from_string("oauth"), status,
                        "Token endpoint returned status %u", status);
        g_object_unref(msg);
        return NULL;
    }

    gsize rlen = 0;
    const char* rdata = (const char*)g_bytes_get_data(resp, &rlen);
    g_printerr("perform_token_exchange: status=%u response=%s\n", status, rdata);

    JsonParser* parser = json_parser_new();
    // Load parsed stream directly without performing a g_strndup clone string step first
    if (!json_parser_load_from_data(parser, rdata, rlen, error))
    {
        g_object_unref(parser);
        g_bytes_unref(resp);
        g_object_unref(msg);
        return NULL;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root))
    {
        g_printerr("Token exchange response root is not an object!\n");
        g_set_error(error, g_quark_from_string("oauth"), 0, "Response root not an object");
        g_object_unref(parser);
        g_bytes_unref(resp);
        g_object_unref(msg);
        return NULL;
    }

    // Reference the internal object tree before unreferencing the parser
    JsonObject* obj = json_object_ref(json_node_get_object(root));

    g_object_unref(parser);
    g_bytes_unref(resp);
    g_object_unref(msg);

    return obj;  // Caller takes ownership via JSON reference
}

static void save_token(sqlite3* db, JsonObject* obj)
{
    if (!json_object_has_member(obj, "access_token"))
    {
        g_printerr("No access_token in JSON, not saving.\n");
        return;
    }

    const gchar* access_token = json_object_get_string_member(obj, "access_token");
    gint expires_in = json_object_get_int_member(obj, "expires_in");
    const gchar* scope = json_object_get_string_member(obj, "scope");
    const gchar* token_type = json_object_get_string_member(obj, "token_type");

    const char* sql =
        "INSERT OR REPLACE INTO token "
        "(id, access_token, expires_in, scope, token_type, created_at) "
        "VALUES (1, ?, ?, ?, ?, strftime('%s','now'));";

    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        g_printerr("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    // SQLITE_STATIC avoids unnecessary internal memory copying inside SQLite
    sqlite3_bind_text(stmt, 1, access_token, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, expires_in);
    sqlite3_bind_text(stmt, 3, scope ? scope : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, token_type ? token_type : "", -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        g_printerr("Failed to insert token: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}

static void on_message(SoupWebsocketConnection* connection, SoupWebsocketDataType type,
                       GBytes* message, gpointer user_data)
{
    AppContext* ctx = (AppContext*)user_data;
    if (type != SOUP_WEBSOCKET_DATA_TEXT) return;

    gsize len = 0;
    const char* data = (const char*)g_bytes_get_data(message, &len);

    JsonParser* parser = json_parser_new();
    GError* err = NULL;
    if (!json_parser_load_from_data(parser, data, len, &err))
    {
        gchar* reply =
            g_strdup_printf("{\"type\":\"exchange_result\",\"ok\":false,\"error\":\"%s\"}",
                            err ? err->message : "parse error");
        soup_websocket_connection_send_text(connection, reply);
        g_free(reply);
        g_clear_error(&err);
        g_object_unref(parser);
        return;
    }

    JsonObject* obj = json_node_get_object(json_parser_get_root(parser));
    const gchar* action = json_object_get_string_member(obj, "action");
    if (g_strcmp0(action, "exchange_code") == 0)
    {
        const gchar* code = json_object_get_string_member(obj, "code");
        const gchar* verifier = json_object_get_string_member(obj, "code_verifier");
        const gchar* client_id = json_object_get_string_member(obj, "client_id");

        GError* xerr = NULL;
        JsonObject* resp_obj =
            perform_token_exchange(ctx->session, code, verifier, client_id, &xerr);
        if (resp_obj)
        {
            save_token(ctx->database, resp_obj);

            JsonNode* node = json_node_new(JSON_NODE_OBJECT);
            json_node_set_object(node, resp_obj);

            gchar* json_str = json_to_string(node, FALSE);
            gchar* reply =
                g_strdup_printf("{\"type\":\"exchange_result\",\"ok\":true,\"data\":%s}", json_str);
            g_free(json_str);
            soup_websocket_connection_send_text(connection, reply);
            g_free(reply);

            json_node_free(node);
            json_object_unref(resp_obj);
        }
        else
        {
            const gchar* msg = xerr ? xerr->message : "Token exchange failed";
            gchar* reply = g_strdup_printf(
                "{\"type\":\"exchange_result\",\"ok\":false,\"error\":\"%s\"}", msg);
            soup_websocket_connection_send_text(connection, reply);
            g_free(reply);
            g_clear_error(&xerr);
        }
    }

    g_object_unref(parser);
}

static gboolean token_is_valid(sqlite3* database)
{
    const char* sql = "SELECT access_token, expires_in, created_at FROM token WHERE id = 1;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        g_printerr("Failed to prepare validity statement: %s\n", sqlite3_errmsg(database));
        return FALSE;
    }

    gboolean valid = FALSE;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char* token = (const char*)sqlite3_column_text(stmt, 0);
        int expires_in = sqlite3_column_int(stmt, 1);
        int created_at = sqlite3_column_int(stmt, 2);

        int now = (int)time(NULL);

        if (created_at + expires_in <= now)
        {
            g_printerr("Token expired, deleting from database.\n");
            sqlite3_exec(database, "DELETE FROM token WHERE id = 1;", NULL, NULL, NULL);
            valid = FALSE;
        }
        else
        {
            g_printerr("Existing token is still valid.\n");
            valid = TRUE;
        }
    }
    else
    {
        g_printerr("No existing token found in database.\n");
        valid = FALSE;
    }

    sqlite3_finalize(stmt);
    return valid;
}

static void socket_handler(SoupServer* server, SoupServerMessage* msg, const char* path,
                           SoupWebsocketConnection* connection, gpointer user_data)
{
    g_printerr("New connection on %s\n", path);

    g_signal_connect(connection, "message", G_CALLBACK(on_message), user_data);
    g_signal_connect(connection, "closed", G_CALLBACK(connection_closed), NULL);

    sqlite3* database = app_context_get_database((AppContext*)user_data);
    if (token_is_valid(database))
    {
        soup_websocket_connection_send_text(connection,
                                            "{\"type\":\"auth_status\",\"logged_in\":true}");
    }
    else
    {
        soup_websocket_connection_send_text(connection,
                                            "{\"type\":\"auth_status\",\"logged_in\":false}");
    }

    // Manually hold a reference to keep the socket alive within the async handler event loop
    g_object_ref(connection);
}

static void connection_closed(SoupWebsocketConnection* connection, gpointer user_data)
{
    gint code = soup_websocket_connection_get_close_code(connection);
    g_printerr("OAuth websocket closed, code=%d\n", code);

    // Unconditional unref ensures we never leak connections on unexpected socket failures
    g_object_unref(connection);
}

static sqlite3* ensure_and_get_database(const gchar* db_path)
{
    sqlite3* db = NULL;
    if (sqlite3_open(db_path, &db) != SQLITE_OK)
    {
        g_printerr("Failed to open database: %s\n", sqlite3_errmsg(db));
        if (db) sqlite3_close(db);
        return NULL;
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS token ("
        "  id INTEGER PRIMARY KEY CHECK (id = 1),"
        "  access_token TEXT NOT NULL,"
        "  expires_in INTEGER NOT NULL,"
        "  scope TEXT NOT NULL,"
        "  token_type TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL"
        ");";

    char* err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK)
    {
        g_printerr("Failed to create table: %s\n", err);
        sqlite3_free(err);
    }

    return db;
}

int main()
{
    gchar* app_dir = g_build_filename(g_get_user_data_dir(), "trader", NULL);
    gchar* db_path = g_build_filename(app_dir, "profile.db", NULL);

    if (g_mkdir_with_parents(app_dir, 0755) == -1)
    {
        g_printerr("Failed to create data directory: %s\n", strerror(errno));
        g_free(app_dir);
        g_free(db_path);
        return 1;
    }

    AppContext* ctx = app_context_new(ensure_and_get_database(db_path), soup_session_new());
    if (!ctx->database)
    {
        g_free(app_dir);
        g_free(db_path);
        return 1;
    }

    SoupServer* server = soup_server_new(NULL, NULL);
    soup_server_add_websocket_handler(server, "/trader", NULL, NULL, socket_handler, &ctx, NULL);

    GError* error = NULL;
    if (!soup_server_listen_all(server, 5000, SOUP_SERVER_LISTEN_IPV4_ONLY, &error))
    {
        g_printerr("Failed to listen: %s\n", error->message);
        g_clear_error(&error);
        sqlite3_close(ctx->database);
        g_object_unref(ctx->session);
        g_object_unref(server);
        g_free(app_dir);
        g_free(db_path);
        return 1;
    }

    g_print("Trader server running on port 5000\n");

    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Structural Clean up
    g_main_loop_unref(loop);
    g_object_unref(server);
    g_object_unref(ctx->session);
    sqlite3_close(ctx->database);
    g_free(app_dir);
    g_free(db_path);
    g_object_unref(ctx);

    return 0;
}
