#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <string.h>
#include <unistd.h>

#include "libsoup/soup-server.h"

static void on_events_message(SoupWebsocketConnection* connection, gint type, GBytes* message,
                              gpointer user_data);
static void oauth_conn_closed(SoupWebsocketConnection* connection, gpointer user_data);

static void ws_events_handler(SoupServer* server, SoupServerMessage* server_msg, const char* path,
                              SoupWebsocketConnection* connection, gpointer user_data)
{
    /* Minimal events handler: echo received text back for debugging */
    g_signal_connect(connection, "message", G_CALLBACK(on_events_message), NULL);
}

static void on_events_message(SoupWebsocketConnection* connection, gint type, GBytes* message,
                              gpointer user_data)
{
    if (type != SOUP_WEBSOCKET_DATA_TEXT) return;
    gsize len = 0;
    const guint8* data = g_bytes_get_data(message, &len);
    gchar* text = g_strndup((const char*)data, len);
    soup_websocket_connection_send_text(connection, text);
    g_free(text);
}

static gchar* perform_token_exchange(const gchar* code, const gchar* code_verifier, GError** error)
{
    SoupSession* session = soup_session_new();
    SoupMessage* msg = soup_message_new("POST", "https://auth.deriv.com/oauth2/token");

    const char* redirect = "https://trader.binclab.com/index";
    gchar* esc_code = g_uri_escape_string(code, NULL, FALSE);
    gchar* esc_verifier = g_uri_escape_string(code_verifier, NULL, FALSE);
    gchar* esc_redirect = g_uri_escape_string(redirect, NULL, FALSE);

    gchar* body =
        g_strdup_printf("grant_type=authorization_code&code=%s&redirect_uri=%s&code_verifier=%s",
                        esc_code, esc_redirect, esc_verifier);

    g_free(esc_code);
    g_free(esc_verifier);
    g_free(esc_redirect);

    GBytes* bytes = g_bytes_new(body, strlen(body));
    soup_message_set_request_body_from_bytes(msg, "application/x-www-form-urlencoded", bytes);
    g_bytes_unref(bytes);
    g_free(body);

    GError* err = NULL;
    GBytes* resp = soup_session_send_and_read(session, msg, NULL, &err);

    gchar* response = NULL;
    guint status = soup_message_get_status(msg);
    if (resp != NULL && status == 200)
    {
        gsize rlen = 0;
        const guint8* rdata = g_bytes_get_data(resp, &rlen);
        response = g_strndup((const char*)rdata, rlen);
        g_bytes_unref(resp);
    }
    else
    {
        if (err)
            g_propagate_error(error, err);
        else
            g_set_error(error, g_quark_from_string("oauth"), status,
                        "Token endpoint returned status %u", status);
    }

    g_object_unref(msg);
    g_object_unref(session);
    return response;
}

static void on_oauth_message(SoupWebsocketConnection* connection, SoupWebsocketDataType type,
                             GBytes* message, gpointer user_data)
{
    gsize len = 0;
    const guint8* data = g_bytes_get_data(message, &len);
    gchar* text = g_strndup((const char*)data, len);
    g_printerr("Received OAuth message (type=%d): %s\n", type, text);

    if (type != SOUP_WEBSOCKET_DATA_TEXT)
    {
        g_free(text);
        return;
    }

    JsonParser* parser = json_parser_new();
    GError* err = NULL;
    if (!json_parser_load_from_data(parser, text, -1, &err))
    {
        gchar* reply =
            g_strdup_printf("{\"type\":\"exchange_result\",\"ok\":false,\"error\":\"%s\"}",
                            err ? err->message : "parse error");
        soup_websocket_connection_send_text(connection, reply);
        g_free(reply);
        g_clear_error(&err);
        g_object_unref(parser);
        g_free(text);
        return;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root))
    {
        g_object_unref(parser);
        g_free(text);
        return;
    }

    JsonObject* obj = json_node_get_object(root);
    const gchar* action = json_object_get_string_member(obj, "action");
    if (g_strcmp0(action, "exchange_code") == 0)
    {
        const gchar* code = json_object_get_string_member(obj, "code");
        const gchar* code_verifier = json_object_get_string_member(obj, "code_verifier");

        GError* xerr = NULL;
        gchar* resp = perform_token_exchange(code, code_verifier, &xerr);
        if (resp)
        {
            gchar* reply =
                g_strdup_printf("{\"type\":\"exchange_result\",\"ok\":true,\"data\":%s}", resp);
            soup_websocket_connection_send_text(connection, reply);
            g_free(reply);
            g_free(resp);
        }
        else
        {
            const gchar* msg = xerr ? xerr->message : "Token exchange failed";
            JsonBuilder* b = json_builder_new();
            json_builder_begin_object(b);
            json_builder_set_member_name(b, "type");
            json_builder_add_string_value(b, "exchange_result");
            json_builder_set_member_name(b, "ok");
            json_builder_add_boolean_value(b, FALSE);
            json_builder_set_member_name(b, "error");
            json_builder_add_string_value(b, msg);
            json_builder_end_object(b);
            JsonGenerator* gen = json_generator_new();
            JsonNode* n = json_builder_get_root(b);
            json_generator_set_root(gen, n);
            gchar* out = json_generator_to_data(gen, NULL);
            soup_websocket_connection_send_text(connection, out);
            g_free(out);
            g_object_unref(gen);
            json_node_free(n);
            g_object_unref(b);
            g_clear_error(&xerr);
        }
    }

    g_object_unref(parser);
    g_free(text);
}

static void ws_oauth_exchange_handler(SoupServer* server, SoupServerMessage* server_msg,
                                      const char* path, SoupWebsocketConnection* connection,
                                      gpointer user_data)
{
    g_printerr("OAuth websocket handler: new connection on path %s\n", path);
    gint state = soup_websocket_connection_get_state(connection);
    g_printerr("  connection state=%d\n", state);
    g_signal_connect(connection, "message", G_CALLBACK(on_oauth_message), NULL);
    g_signal_connect(connection, "closed", G_CALLBACK(oauth_conn_closed), NULL);
    /* Send a welcome ping so client can see server-side reachability */
    soup_websocket_connection_send_text(connection, "{\"type\":\"welcome\"}");
    g_printerr("Sent welcome message to client\n");
}

static void oauth_conn_closed(SoupWebsocketConnection* connection, gpointer user_data)
{
    (void)connection;
    (void)user_data;
    gint state = soup_websocket_connection_get_state(connection);
    gint code = soup_websocket_connection_get_close_code(connection);
    g_printerr("OAuth websocket connection closed: state=%d code=%d\n", state, code);
}

int main()
{
    gchar* dir = g_build_filename(g_get_user_data_dir(), "trader", NULL);
    g_mkdir_with_parents(dir, 0755);
    gchar* db_path = g_build_filename(dir, "profile.db", NULL);
    GError* error = NULL;

    int result = access("/var/www/server.crt", R_OK);
    char* cert_path = result == 0 ? "/var/www/server.crt" : "/etc/ssl/certs/server.crt";
    char* key_path = result == 0 ? "/var/www/server.key" : "/etc/ssl/private/server.key";
    GTlsCertificate* cert = g_tls_certificate_new_from_files(cert_path, key_path, &error);

    SoupServer* server = soup_server_new("tls-certificate", cert, NULL);
    soup_server_add_websocket_handler(server, "/events", NULL, NULL, ws_events_handler, NULL, NULL);
    soup_server_add_websocket_handler(server, "/oauth/exchange", NULL, NULL,
                                      ws_oauth_exchange_handler, NULL, NULL);

    if (!soup_server_listen_all(server, 5000, SOUP_SERVER_LISTEN_IPV4_ONLY, &error))
    {
        g_printerr("Failed to listen (plain WS): %s\n", error ? error->message : "unknown");
    }
    if (!soup_server_listen_all(server, 5001, SOUP_SERVER_LISTEN_HTTPS, &error))
    {
        g_printerr("Failed to listen: %s\n", error ? error->message : "unknown");
        return 1;
    }

    g_print("Trader server running on https://0.0.0.0:5000\n");

    // Keep the server alive
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_object_unref(server);
    g_object_unref(cert);
    return 0;
}
