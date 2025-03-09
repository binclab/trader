#include "session.h"

const SecretSchema *get_token_schema(void) {
    static const SecretSchema schema = {
        "com.binclab.Trader",
        SECRET_SCHEMA_NONE,
        {{"account", SECRET_SCHEMA_ATTRIBUTE_STRING},
         {"currency", SECRET_SCHEMA_ATTRIBUTE_STRING},
         {NULL, 0}}};
    return &schema;
};

static void store_token_result(GObject *source, GAsyncResult *result, gpointer userdata)
{
    GError *error = NULL;
    secret_password_store_finish(result, &error);
    if (error)
    {
        g_printerr("Failed to store token: %s\n", error->message);
        g_error_free(error);
    }

    g_clear_pointer(&userdata, g_free);
}

void load_changed(WebKitWebView *webview, WebKitLoadEvent event, gpointer userdata)
{
    GObject *task = G_OBJECT(userdata);
    switch (event)
    {
    case WEBKIT_LOAD_STARTED:
        break;
    case WEBKIT_LOAD_REDIRECTED:
        const gchar *url = webkit_web_view_get_uri(webview);
        GListStore *profile = G_LIST_STORE(g_object_get_data(task, "profile"));
        if (g_strstr_len(url, -1, "https://www.binclab.com/deriv?"))
        {
            gchar **result = g_strsplit(g_strrstr(url, "acct") + 4, "=", 0);
            gchar *endptr;
            gint count = g_strtod(result[0], &endptr);
            g_strfreev(result);
            result = g_strsplit(g_strstr_len(url, -1, "acct1"), "&", 0);
            for (gint index = 0; index < count; index++)
            {
                gint position = index * 3;
                gchar *account = g_strdup(g_strrstr(result[position], "=") + 1);
                gchar *token = g_strdup(g_strrstr(result[position + 1], "=") + 1);
                gchar *currency = g_strdup(g_strrstr(result[position + 2], "=") + 1);
                GObject *object = G_OBJECT(gtk_string_object_new(account));
                g_object_set_data(object, "token", token);
                g_object_set_data(object, "currency", currency);
                g_list_store_append(profile, object);

                secret_password_store(get_token_schema(), SECRET_COLLECTION_DEFAULT,
                                      "Binc Trader Token", token, NULL,
                                      store_token_result, account,
                                      "account", account,
                                      "currency", currency,
                                      NULL);
            }
            g_strfreev(result);
            g_task_run_in_thread(G_TASK(task), save_token_attributes);
            present_actual_child(task);
        }
        //}
        break;
    case WEBKIT_LOAD_COMMITTED:
        break;
    case WEBKIT_LOAD_FINISHED:
        break;
    }
}


static gchar *request_tick_history(const gchar *symbol, gint size)
{
    JsonNode *node = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();

    json_object_set_string_member(object, "ticks_history", symbol);
    json_object_set_int_member(object, "adjust_start_time", 1);
    json_object_set_int_member(object, "count", size);
    json_object_set_string_member(object, "end", "latest");
    json_object_set_string_member(object, "style", "candles");
    json_object_set_int_member(object, "granularity", 60);
    json_object_set_int_member(object, "start", 1);
    json_object_set_int_member(object, "subscribe", 1);

    json_node_init(node, JSON_NODE_OBJECT);
    json_node_take_object(node, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, node);

    gchar *instruction = json_generator_to_data(generator, NULL);

    g_object_unref(generator);

    return instruction;
}

static gchar *request_active_symbols()
{
    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();

    json_object_set_string_member(object, "active_symbols", "full");
    json_object_set_string_member(object, "product_type", "basic");
    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);

    return json_generator_to_data(generator, NULL);
}

static void process_candle_history(JsonArray *array, guint length, GObject *task)
{
    double highest = 0;
    double lowest = 0;

    GListStore *store = g_object_get_data(task, "candles");
    gint skip = g_list_model_get_n_items(G_LIST_MODEL(store));

    for (size_t index = 0; index < 1440; index++)
    {
        GObject *candle;
        if (index >= skip)
        {
            JsonObject *object = json_node_get_object(json_array_get_element(array, index - skip));

            candle = g_object_new(G_TYPE_OBJECT, NULL);
            GDateTime *epoch = g_date_time_new_from_unix_utc(json_object_get_int_member(object, "epoch"));
            gdouble *price = g_new(gdouble, 4);
            price[0] = json_object_get_double_member(object, "open");
            price[1] = json_object_get_double_member(object, "close");
            price[2] = json_object_get_double_member(object, "high");
            price[3] = json_object_get_double_member(object, "low");

            g_object_set_data(candle, "price", price);
            g_object_set_data(candle, "epoch", epoch);
            g_object_set_data(candle, "stat", g_object_get_data(task, "stat"));
            g_object_set_data(candle, "data", g_object_get_data(task, "data"));
            g_object_set_data(candle, "time", g_object_get_data(task, "time"));

            json_object_unref(object);

            g_list_store_append(store, candle);
        }
        else
        {
            candle = g_list_model_get_item(G_LIST_MODEL(store), index);
        }

        gdouble *price = (gdouble *)g_object_get_data(candle, "price");

        if (highest < price[2])
            highest = price[2];

        if ((price[3] < lowest) || (lowest == 0))
            lowest = price[3];

        if (index == 1439)
        {
            highest = highest;
            lowest = lowest;

            gdouble *stat = g_object_get_data(task, "stat");
            g_object_set_data(task, "price", price);
            stat[0] = highest - lowest;
            stat[1] = highest;
            stat[2] = lowest;
            stat[3] = lowest + stat[0] / 2;
            stat[4] = 2 / stat[0];
            // bincdata->stat->scale = 0.25 / bincdata->stat->range;
        }
    }
}

static void create_chart(GObject *task)
{
}

static void handle_socket_response(JsonObject *object, const gchar *type, GTask *task)
{
    GObject *pointer = G_OBJECT(task);
    if (g_str_equal("candles", type))
    {
        JsonArray *array = json_object_get_array_member(object, "candles");
        guint length = json_array_get_length(array);
        JsonObject *request = json_object_get_object_member(object, "echo_req");
        gint count = json_object_get_int_member(request, "count");
        gint start = json_object_get_int_member(request, "start");
        const gchar *end = json_object_get_string_member(request, "end");
        gchar *endptr;
        if ((gint)g_strtod(end, &endptr) == length && length == start && start == count)
        {
            JsonObject *current = json_array_get_object_element(array, 0);
            /*GtkGLArea *area = g_object_get_data(G_OBJECT(bincdata->widget), "area");
            BincCandle *candle = g_object_get_data(G_OBJECT(area), "candle");
            candle->price->close = json_object_get_double_member(current, "close");
            candle->price->epoch = json_object_get_int_member(current, "epoch");
            candle->price->high = json_object_get_double_member(current, "high");
            candle->price->low = json_object_get_double_member(current, "low");
            candle->price->open = json_object_get_double_member(current, "open");
            // gtk_gl_area_queue_render(GTK_GL_AREA(bincdata->lastwidget));
            gtk_gl_area_queue_render(area);
            bincdata->count = 1;
            g_list_store_append(bincdata->store, candle);
            g_task_run_in_thread(bincdata->task, save_history);*/

            /*AccountProfile **profile = (AccountProfile *)g_object_get_data(task, "profile");
            BincSymbol *instrument = (BincSymbol *)g_object_get_data(task, "symbol");*/
        }
        else
        {
            process_candle_history(array, length, pointer);
            g_task_set_task_data(task, GINT_TO_POINTER(length), NULL);
            g_task_run_in_thread(task, save_history);
            create_chart(pointer);
        }
    }
    else if (g_str_equal("ohlc", type))
    {
        // update_candle(bincdata, json_object_get_object_member(object, "ohlc"));
    }
    else if (g_str_equal("time", type))
    {
    }
    else if (g_str_equal("active_symbols", type))
    {
        update_active_symbols(pointer, json_object_get_array_member(object, "active_symbols"));
        // setup_instrument(bincdata);
        /*if (window->model->instrument == NULL)
        {
            set_default_instrument(window->home);
            // window->model->instrument = restore_last_instrument(window->home);
        }*/
    }
}

static void message(SoupWebsocketConnection *connection, SoupWebsocketDataType type, GBytes *message, gpointer userdata)
{
    if (type == SOUP_WEBSOCKET_DATA_TEXT)
    {
        gsize size;
        gchar *data = (gchar *)g_bytes_get_data(message, &size);
        JsonParser *parser = json_parser_new();
        GError *error = NULL;
        if (!json_parser_load_from_data(parser, data, size, &error))
        {
            g_printerr("Unable to parse JSON: %s\n", error->message);
            g_error_free(error);
            g_object_unref(parser);
        }
        else
        {
            JsonObject *object = json_node_get_object(json_parser_get_root(parser));
            const gchar *type = json_object_get_string_member(object, "msg_type");
            handle_socket_response(object, type, G_TASK(userdata));
        }
    }
    else
    {
        g_print("Received binary message\n");
    }
    // object->bytes_recieved += g_bytes_get_size(message);
    // char buffer[16];
    // snprintf(buffer, 16, "Used %.3f MB", object->bytes_recieved / 1048576.0);
    // if (object->infolabel != NULL)
    //     gtk_label_set_text(window->infolabel, buffer);
}

static void closed(SoupWebsocketConnection *connection, gpointer *userdata)
{
    gpointer pointer = g_object_get_data(G_OBJECT(userdata), "connection");
    if (pointer)
    {
        g_object_unref(pointer);
    }
}

void connected(GObject *object, GAsyncResult *result, gpointer userdata)
{
    GObject *task = G_OBJECT(userdata);
    SoupSession *session = SOUP_SESSION(object);
    GError *error = NULL;
    SoupWebsocketConnection *connection = soup_session_websocket_connect_finish(session, result, &error);

    if (error)
    {
        g_printerr("Failed to connect to WebSocket: %s\n", error->message);
        g_error_free(error);
        return;
    }

    if (!g_object_get_data(task, "connection"))
    {
        g_object_set_data(task, "connection", connection);
    }

    gpointer pointer = g_object_get_data(task, "symbol");

    if (g_object_get_data(task, "fetch"))
    {
        gchar *instruction = request_active_symbols();
        soup_websocket_connection_send_text(connection, instruction);
    }
    else if (pointer)
    {
        const gchar *symbol = gtk_string_object_get_string(GTK_STRING_OBJECT(pointer));
        pointer = g_object_get_data(task, "timeframe");
        const gchar *timeframe = gtk_string_object_get_string(GTK_STRING_OBJECT(pointer));
        gint count = get_saved_candles(task, symbol, timeframe);
        gchar *instruction = request_tick_history(symbol, count);
        soup_websocket_connection_send_text(connection, instruction);
    }

    g_signal_connect(connection, "message", G_CALLBACK(message), userdata);
    g_signal_connect(connection, "closed", G_CALLBACK(closed), userdata);
}