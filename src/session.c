#include "session.h"
#include "chart.h"

const SecretSchema *get_token_schema(void)
{
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
    GObject *object = G_OBJECT(userdata);
    switch (event)
    {
    case WEBKIT_LOAD_STARTED:
        break;
    case WEBKIT_LOAD_REDIRECTED:
        const gchar *url = webkit_web_view_get_uri(webview);
        GListStore *profile = G_LIST_STORE(g_object_get_data(object, "profile"));
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
                GObject *item = G_OBJECT(gtk_string_object_new(account));
                g_object_set_data(item, "token", token);
                g_object_set_data(item, "currency", currency);
                g_list_store_append(profile, item);

                secret_password_store(get_token_schema(), SECRET_COLLECTION_DEFAULT,
                                      "Binc Trader Token", token, NULL,
                                      store_token_result, account,
                                      "account", account,
                                      "currency", currency,
                                      NULL);
            }
            g_strfreev(result);
            GTask *task = g_task_new(object, NULL, NULL, NULL);
            g_task_set_task_data(task, profile, NULL);
            g_task_run_in_thread(task, save_token_attributes);
            g_object_unref(task);
            present_actual_child(object);
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

static void process_candle_history(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    JsonArray *array = (JsonArray *)userdata;
    guint length = json_array_get_length(array);

    GObject *object = G_OBJECT(source);
    GObject *stat = G_OBJECT(g_object_get_data(object, "stat"));
    GObject *symbol = G_OBJECT(g_object_get_data(object, "symbol"));
    gfloat *highest = (gfloat *)g_object_get_data(stat, "highest");
    gfloat *lowest = (gfloat *)g_object_get_data(stat, "lowest");
    gfloat *spot = (gfloat *)g_object_get_data(symbol, "spot");
    gfloat *pip = (gfloat *)g_object_get_data(symbol, "pip");
    GListStore *store = g_object_get_data(object, "candles");
    gint skip = g_list_model_get_n_items(G_LIST_MODEL(store));

    for (size_t index = 0; index < 1440; index++)
    {
        GObject *candle;
        if (index >= skip)
        {
            JsonObject *item = json_node_get_object(json_array_get_element(array, index - skip));

            candle = g_object_new(G_TYPE_OBJECT, NULL);
            GDateTime *epoch = g_date_time_new_from_unix_utc(json_object_get_int_member(item, "epoch"));
            gfloat *open = g_new0(gfloat, 1);
            gfloat *close = g_new0(gfloat, 1);
            gfloat *high = g_new0(gfloat, 1);
            gfloat *low = g_new0(gfloat, 1);
            *open = (gfloat)json_object_get_double_member(item, "open");
            *close = (gfloat)json_object_get_double_member(item, "close");
            *high = (gfloat)json_object_get_double_member(item, "high");
            *low = (gfloat)json_object_get_double_member(item, "low");

            g_object_set_data(candle, "open", open);
            g_object_set_data(candle, "close", close);
            g_object_set_data(candle, "high", high);
            g_object_set_data(candle, "low", low);
            g_object_set_data(candle, "epoch", epoch);
            g_object_set_data(candle, "stat", g_object_get_data(object, "stat"));
            g_object_set_data(candle, "data", g_object_get_data(object, "data"));
            g_object_set_data(candle, "time", g_object_get_data(object, "time"));

            json_object_unref(item);

            g_list_store_append(store, candle);
        }
        else
        {
            candle = g_list_model_get_item(G_LIST_MODEL(store), index);
        }

        gfloat *high = (gfloat *)g_object_get_data(candle, "high");
        gfloat *low = (gfloat *)g_object_get_data(candle, "low");

        if (*highest < *high)
            *highest = *high;

        if ((*low < *lowest) || (lowest == 0))
            *lowest = *low;

        if (index == 1439)
        {
            gfloat *range = (gfloat *)g_object_get_data(stat, "range");
            gfloat *baseline = (gfloat *)g_object_get_data(stat, "baseline");
            gfloat *factor = (gfloat *)g_object_get_data(stat, "factor");
            gfloat *open = (gfloat *)g_object_get_data(candle, "open");
            gfloat *close = (gfloat *)g_object_get_data(candle, "close");
            g_object_set_data(object, "open", open);
            g_object_set_data(object, "close", close);
            g_object_set_data(object, "high", high);
            g_object_set_data(object, "low", low);
            *range = *highest - *lowest;
            *baseline = *open;
            *factor = 16 / (*pip * 1000);
            // bincdata->stat->scale = 0.25 / bincdata->stat->range;
        }
    }
    if (GTK_IS_FIXED(g_object_get_data(object, "chartfixed")))
    {
        GTask *subtask = g_task_new(object, NULL, que_widgets, NULL);
        g_task_run_in_thread(subtask, create_chart);
        g_object_unref(subtask);
    }
    GTask *subtask = g_task_new(object, NULL, NULL, NULL);
    g_task_set_task_data(subtask, GINT_TO_POINTER(length), NULL);
    g_task_run_in_thread(subtask, save_history);
    g_object_unref(subtask);
}

static void handle_socket_response(JsonObject *element, const gchar *type, GObject *object)
{
    if (g_str_equal("candles", type))
    {
        JsonArray *array = json_object_get_array_member(element, "candles");
        guint length = json_array_get_length(array);
        JsonObject *request = json_object_get_object_member(element, "echo_req");
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
            GTask *task = g_task_new(object, NULL, NULL, NULL);
            g_task_set_task_data(task, array, NULL);
            g_task_run_in_thread(task, process_candle_history);
            g_object_unref(task);
        }
    }
    else if (g_str_equal("ohlc", type))
    {
        // update_candle(bincdata, json_object_get_object_member(element, "ohlc"));
    }
    else if (g_str_equal("time", type))
    {
    }
    else if (g_str_equal("active_symbols", type))
    {
        update_active_symbols(object, json_object_get_array_member(element, "active_symbols"));
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
    GObject *object = G_OBJECT(userdata);
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
            JsonObject *element = json_node_get_object(json_parser_get_root(parser));
            const gchar *type = json_object_get_string_member(element, "msg_type");
            handle_socket_response(element, type, object);
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

void connected(GObject *source, GAsyncResult *result, gpointer userdata)
{
    GObject *object = G_OBJECT(userdata);
    SoupSession *session = SOUP_SESSION(source);
    GError *error = NULL;
    SoupWebsocketConnection *connection = soup_session_websocket_connect_finish(session, result, &error);
    g_signal_connect(connection, "message", G_CALLBACK(message), userdata);
    g_signal_connect(connection, "closed", G_CALLBACK(closed), userdata);

    if (error)
    {
        g_printerr("Failed to connect to WebSocket: %s\n", error->message);
        g_error_free(error);
        return;
    }

    if (!g_object_get_data(object, "connection"))
    {
        g_object_set_data(object, "connection", connection);
    }

    gpointer pointer = g_object_get_data(object, "symbol");

    if (soup_websocket_connection_get_state(connection) != SOUP_WEBSOCKET_STATE_OPEN)
    {
        g_warning("WebSocket connection is not open.");
        return;
    }

    if (g_object_get_data(object, "fetch"))
    {
        gchar *instruction = request_active_symbols();
        soup_websocket_connection_send_text(connection, instruction);
    }
    else if (pointer)
    {
        const gchar *symbol = gtk_string_object_get_string(GTK_STRING_OBJECT(pointer));
        pointer = g_object_get_data(object, "timeframe");
        const gchar *timeframe = gtk_string_object_get_string(GTK_STRING_OBJECT(pointer));
        gint count = get_saved_candles(object, symbol, timeframe);
        gchar *instruction = request_tick_history(symbol, count);
        soup_websocket_connection_send_text(connection, instruction);
    }
}