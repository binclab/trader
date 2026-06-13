#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <string.h>
#include <unistd.h>

static void on_events_message(SoupWebsocketConnection* connection, SoupWebsocketDataType type,
                              GBytes* message, gpointer user_data);
static void on_oauth_message(SoupWebsocketConnection* connection, SoupWebsocketDataType type,
                             GBytes* message, gpointer user_data);
static void oauth_conn_closed(SoupWebsocketConnection* connection, gpointer user_data);

static void ws_events_handler(SoupServer* server, SoupServerMessage* msg, const char* path,
                              SoupWebsocketConnection* connection, gpointer user_data)
{
    g_signal_connect(connection, "message", G_CALLBACK(on_events_message), NULL);
}

static void on_events_message(SoupWebsocketConnection* connection, SoupWebsocketDataType type,
                              GBytes* message, gpointer user_data)
{
    if (type != SOUP_WEBSOCKET_DATA_TEXT) return;
    gsize len = 0;
    const guint8* data = g_bytes_get_data(message, &len);
    gchar* text = g_strndup((const char*)data, len);
    soup_websocket_connection_send_text(connection, text);
    g_free(text);
}

static gchar* perform_token_exchange(const gchar* code, const gchar* code_verifier,
                                     const gchar* client_id, GError** error)
{
    g_printerr("perform_token_exchange: code=%s verifier=%s\n", code, code_verifier);
    // Instead of calling the real endpoint, just return dummy JSON
    SoupSession* session = soup_session_new();
    SoupMessage* msg = soup_message_new("POST", "https://auth.deriv.com/oauth2/token");

    const char* redirect = "https://trader.binclab.com/index";
    gchar* body;
    if (client_id && *client_id)
        body = g_strdup_printf(
            "grant_type=authorization_code&code=%s&redirect_uri=%s&code_verifier=%s&client_id=%s",
            code, redirect, code_verifier, client_id);
    else
        body = g_strdup_printf(
            "grant_type=authorization_code&code=%s&redirect_uri=%s&code_verifier=%s", code,
            redirect, code_verifier);
    g_printerr("perform_token_exchange: request body: %s\n", body);

    GBytes* bytes = g_bytes_new(body, strlen(body));
    soup_message_set_request_body_from_bytes(msg, "application/x-www-form-urlencoded", bytes);
    g_bytes_unref(bytes);
    g_free(body);

    GError* err = NULL;
    GBytes* resp = soup_session_send_and_read(session, msg, NULL, &err);

    gchar* response = NULL;
    guint status = soup_message_get_status(msg);
    if (resp != NULL)
    {
        gsize rlen = 0;
        const guint8* rdata = g_bytes_get_data(resp, &rlen);
        response = g_strndup((const char*)rdata, rlen);
        g_printerr("perform_token_exchange: status=%u response=%s\n", status, response);
        g_bytes_unref(resp);
    }
    else
    {
        if (resp != NULL)
        {
            gsize rlen = 0;
            const guint8* rdata = g_bytes_get_data(resp, &rlen);
            gchar* errbody = g_strndup((const char*)rdata, rlen);
            g_printerr("perform_token_exchange: non-200 status=%u response=%s\n", status, errbody);
            g_free(errbody);
            g_bytes_unref(resp);
        }
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
    if (type != SOUP_WEBSOCKET_DATA_TEXT) return;
    gsize len = 0;
    const guint8* data = g_bytes_get_data(message, &len);
    gchar* text = g_strndup((const char*)data, len);
    g_printerr("on_oauth_message fired, type=%d\n", type);
    g_printerr("on_oauth_message: received text: %s\n", text);

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

    JsonObject* obj = json_node_get_object(json_parser_get_root(parser));
    const gchar* action = json_object_get_string_member(obj, "action");
    g_printerr("on_oauth_message: parsed action=%s\n", action);
    if (g_strcmp0(action, "exchange_code") == 0)
    {
        const gchar* code = json_object_get_string_member(obj, "code");
        const gchar* verifier = json_object_get_string_member(obj, "code_verifier");
        g_printerr("on_oauth_message: exchange_code: code=%s verifier=%s\n", code, verifier);

        GError* xerr = NULL;
        const gchar* client_id = json_object_get_string_member(obj, "client_id");
        gchar* resp = perform_token_exchange(code, verifier, client_id, &xerr);
        if (resp)
        {
            gchar* reply =
                g_strdup_printf("{\"type\":\"exchange_result\",\"ok\":true,\"data\":%s}", resp);
            g_printerr("on_oauth_message: sending success reply: %s\n", reply);
            soup_websocket_connection_send_text(connection, reply);
            g_free(reply);
            g_free(resp);
        }
        else
        {
            const gchar* msg = xerr ? xerr->message : "Token exchange failed";
            gchar* reply = g_strdup_printf(
                "{\"type\":\"exchange_result\",\"ok\":false,\"error\":\"%s\"}", msg);
            g_printerr("on_oauth_message: sending error reply: %s\n", reply);
            soup_websocket_connection_send_text(connection, reply);
            g_free(reply);
            g_clear_error(&xerr);
        }
    }

    g_object_unref(parser);
    g_free(text);
}

static void ws_oauth_exchange_handler(SoupServer* server, SoupServerMessage* msg, const char* path,
                                      SoupWebsocketConnection* connection, gpointer user_data)
{
    g_printerr("OAuth websocket handler: new connection on %s\n", path);

    // Connect signals for incoming messages and closed event
    g_signal_connect(connection, "message", G_CALLBACK(on_oauth_message), NULL);
    g_signal_connect(connection, "closed", G_CALLBACK(oauth_conn_closed), NULL);

    // Send initial welcome
    soup_websocket_connection_send_text(connection, "{\"type\":\"welcome\"}");

    g_object_ref(connection);
}

static void oauth_conn_closed(SoupWebsocketConnection* connection, gpointer user_data)
{
    gint code = soup_websocket_connection_get_close_code(connection);
    g_printerr("OAuth websocket closed, code=%d\n", code);
    if (code != SOUP_WEBSOCKET_CLOSE_NORMAL) g_object_unref(connection);
}

int main()
{
    SoupServer* server = soup_server_new(NULL, NULL);
    soup_server_add_websocket_handler(server, "/events", NULL, NULL, ws_events_handler, NULL, NULL);
    soup_server_add_websocket_handler(server, "/trader/oauth", NULL, NULL,
                                      ws_oauth_exchange_handler, NULL, NULL);

    GError* error = NULL;
    if (!soup_server_listen_all(server, 5000, SOUP_SERVER_LISTEN_IPV4_ONLY, &error))
    {
        g_printerr("Failed to listen: %s\n", error->message);
        return 1;
    }

    g_print("Trader server running port 5000\n");

    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_object_unref(server);
    return 0;
}