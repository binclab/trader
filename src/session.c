#include "session.h"
#include "cleanup.h"

void get_candle_history(JsonArray *array, guint length, BincData *bincdata)
{
    double highest = 0;
    double lowest = 0;
    size_t skip = bincdata->count - length;

    for (size_t index = 0; index < bincdata->count; index++)
    {
        BincCandle *candle;
        if (index >= skip)
        {
            JsonObject *object = json_node_get_object(json_array_get_element(array, index - skip));

            candle = g_object_new(BINC_TYPE_CANDLE, NULL);
            candle->data = bincdata->data;
            candle->time = bincdata->time;
            candle->stat = bincdata->stat;

            candle->price->close = json_object_get_double_member(object, "close");
            candle->price->epoch = json_object_get_int_member(object, "epoch");
            candle->price->high = json_object_get_double_member(object, "high");
            candle->price->low = json_object_get_double_member(object, "low");
            candle->price->open = json_object_get_double_member(object, "open");
            json_object_unref(object);

            g_list_store_append(bincdata->store, candle);
            if (index == bincdata->count - 1)
            {
                bincdata->price = candle->price;
            }
        }
        else
            candle = g_list_model_get_item((GListModel *)bincdata->store, index);

        if (highest < candle->price->high)
            highest = candle->price->high;
        if ((candle->price->low < lowest) || (lowest == 0))
            lowest = candle->price->low;

        if (index == bincdata->count - 1)
        {
            highest = highest;
            lowest = lowest;
            bincdata->stat->range = highest - lowest;
            bincdata->stat->highest = highest;
            bincdata->stat->lowest = lowest;
            bincdata->stat->baseline = lowest + bincdata->stat->range / 2;
            bincdata->stat->scale = 2 / bincdata->stat->range;
            // bincdata->stat->scale = 0.25 / bincdata->stat->range;
        }
    }
}

gchar *request_active_symbols()
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

gchar *request_ping()
{
    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();

    json_object_set_int_member(object, "ping", 1);

    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);

    return json_generator_to_data(generator, NULL);
}

gchar *request_subscription(gchar *symbol)
{

    /*JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();

    json_object_set_string_member(object, "ticks", symbol);
    json_object_set_int_member(object, "subscribe", 1);

    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);*/

    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();

    json_object_set_string_member(object, "ticks_history", symbol);
    json_object_set_int_member(object, "adjust_start_time", 1);
    json_object_set_int_member(object, "count", 1);
    json_object_set_string_member(object, "end", "latest");
    json_object_set_string_member(object, "style", "candles");
    json_object_set_int_member(object, "granularity", 60);
    json_object_set_int_member(object, "start", 1);
    json_object_set_int_member(object, "subscribe", 1);

    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);

    return json_generator_to_data(generator, NULL);
}

gchar *request_login()
{
    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();
    JsonArray *array = json_array_new();

    json_array_add_string_element(array, "read");
    json_array_add_string_element(array, "trade");

    json_object_set_int_member(object, "api_token", 1);
    json_object_set_string_member(object, "new_token", "Login");
    json_object_set_array_member(object, "new_token_scopes", array);

    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);

    return json_generator_to_data(generator, NULL);
}

gchar *request_last_candle(gchar *symbol)
{

    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();

    json_object_set_string_member(object, "ticks_history", symbol);
    json_object_set_int_member(object, "adjust_start_time", 1);
    json_object_set_int_member(object, "count", 2);
    json_object_set_string_member(object, "end", "2");
    json_object_set_string_member(object, "style", "candles");
    json_object_set_int_member(object, "start", 2);

    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);

    return json_generator_to_data(generator, NULL);
}

gchar *request_tick_history(gchar *symbol, int size)
{
    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();

    json_object_set_string_member(object, "ticks_history", symbol);
    json_object_set_int_member(object, "adjust_start_time", 1);
    json_object_set_int_member(object, "count", size);
    json_object_set_string_member(object, "end", "latest");
    json_object_set_string_member(object, "style", "candles");
    json_object_set_int_member(object, "granularity", 60);
    json_object_set_int_member(object, "start", 1);
    json_object_set_int_member(object, "subscribe", 1);

    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);

    return json_generator_to_data(generator, NULL);
}

gchar *request_time()
{
    JsonNode *root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *object = json_object_new();
    json_object_set_int_member(object, "time", 1);
    json_node_init(root, JSON_NODE_OBJECT);
    json_node_take_object(root, object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);

    return json_generator_to_data(generator, NULL);
}

BincTick *get_tick(gchar *data, gssize size)
{
    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    if (!json_parser_load_from_data(parser, data, size, &error))
    {
        g_printerr("Unable to parse JSON: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return NULL;
    }
    else
    {
        JsonObject *object = json_node_get_object(json_parser_get_root(parser));
        JsonObject *tickObject = json_object_get_object_member(object, "tick");
        BincTick *tick = g_new0(BincTick, 1);
        tick->ask = json_object_get_double_member(tickObject, "ask");
        tick->bid = json_object_get_double_member(tickObject, "bid");
        tick->epoch = json_object_get_int_member(tickObject, "epoch");
        tick->id = json_object_get_string_member(tickObject, "id");
        tick->pip_size = json_object_get_int_member(tickObject, "pip_size");
        tick->quote = json_object_get_double_member(tickObject, "quote");
        tick->symbol = json_object_get_string_member(tickObject, "symbol");
        return tick;
    }
}

void update_candle(BincData *bincdata, JsonObject *object)
{
    gint64 epoch = json_object_get_int_member(object, "epoch");
    GDateTime *current_time = g_date_time_new_from_unix_utc(epoch);

    if (g_date_time_get_second(current_time) == 0)
    {
        BincCandle *candle = g_object_new(BINC_TYPE_CANDLE, NULL);
        candle->data = bincdata->data;
        candle->time = bincdata->time;
        candle->stat = bincdata->stat;
        gchar *endptr;
        candle->price->high = g_strtod(json_object_get_string_member(object, "high"), &endptr);
        candle->price->low = g_strtod(json_object_get_string_member(object, "low"), &endptr);
        candle->price->open = g_strtod(json_object_get_string_member(object, "open"), &endptr);
        candle->price->close = g_strtod(json_object_get_string_member(object, "close"), &endptr);
        candle->price->epoch = epoch;

        const gchar *instruction = request_last_candle(bincdata->instrument->symbol);
        soup_websocket_connection_send_text(bincdata->connection, instruction);
        add_candle(bincdata, candle, TRUE);
        // gtk_fixed_put(fixed, bincdata->widget, 0, 0);
        // bincdata->widget = create_canvas(candle);
        // gtk_box_append(bincdata->data->chart, bincdata->widget);
        // gtk_fixed_put(candle->data->fixed, bincdata->widget, candle->data->abscissa, candle->price->close - candle->data->baseline);
        // candle->data->abscissa += CANDLE_WIDTH;
        gtk_range_set_value(bincdata->data->scale, candle->price->close);
        // gtk_viewport_scroll_to(bincdata->timeport, bincdata->timegravity, bincdata->scrollinfo);
        // gtk_adjustment_set_value(bincdata->adjustment, gtk_adjustment_get_upper(bincdata->adjustment));
        gtk_widget_grab_focus(bincdata->timegravity);
    }
    if (bincdata->widget != NULL && GTK_IS_WIDGET(bincdata->widget))
    {
        gchar *endptr;
        bincdata->price->high = g_strtod(json_object_get_string_member(object, "high"), &endptr);
        bincdata->price->low = g_strtod(json_object_get_string_member(object, "low"), &endptr);
        bincdata->price->open = g_strtod(json_object_get_string_member(object, "open"), &endptr);
        bincdata->price->close = g_strtod(json_object_get_string_member(object, "close"), &endptr);
        bincdata->price->epoch = epoch;
        gtk_range_set_value(bincdata->data->scale, bincdata->price->close);
        // gtk_paned_set_position(bincdata->data->paned, 12 * (bincdata->stat->lowest - bincdata->price->close));
        gtk_gl_area_queue_render(GTK_GL_AREA(bincdata->widget));
        gtk_gl_area_queue_render(bincdata->pointer);
        gtk_gl_area_queue_render(bincdata->cartesian);

        double close_value = bincdata->price->close;

        gchar *buffer = g_strdup_printf("%.*f", bincdata->instrument->pip, close_value);
        if (bincdata->scalelabel != NULL)
            gtk_label_set_text(bincdata->scalelabel, buffer);
        g_free(buffer);

        /*if (close_value >= (bincdata->data->maximum - bincdata->data->space))
        {
            int count = (int)ceil((close_value - bincdata->data->maximum) / bincdata->data->space);
            for (int index = 0; index < count; index++)
            {
                // height += MARKER_HEIGHT;
                // gtk_widget_set_size_request((GtkWidget *)range, width, height);
                // bincdata->data->upper += bincdata->data->space;
                // buffer = g_strdup_printf("%.3f", bincdata->data->upper);
                // gtk_scale_add_mark((GtkScale *)range, bincdata->data->upper, GTK_POS_BOTTOM, buffer);
                create_scale(bincdata->data, index, TRUE);
                // g_free(buffer);
            }

            // gtk_adjustment_set_upper(adjustment, bincdata->data->upper);
        }
        else if (close_value <= (bincdata->data->minimum + bincdata->data->space))
        {
            int count = (int)floor((bincdata->data->minimum - close_value) / bincdata->data->space);
            for (int index = 0; index < count; index++)
            {
                // height += MARKER_HEIGHT;
                // gtk_widget_set_size_request((GtkWidget *)range, width, height);
                // bincdata->data->lower -= bincdata->data->space;
                // buffer = g_strdup_printf("%.3f", bincdata->data->lower);
                // gtk_scale_add_mark((GtkScale *)range, bincdata->data->lower, GTK_POS_BOTTOM, buffer);
                create_scale(bincdata->data, index, FALSE);
                // g_free(buffer);
            }
            // gtk_adjustment_set_lower(adjustment, bincdata->data->lower);
        }*/
    }

    g_date_time_unref(current_time);
}

static void handle_socket_response(JsonObject *object, const gchar *type, BincData *bincdata)
{
    // GDateTime *date = g_date_time_new_now_utc();
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
            GtkGLArea *area = g_object_get_data(G_OBJECT(bincdata->widget), "area");
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
            g_task_run_in_thread(bincdata->task, save_history);
        }
        else
        {
            get_candle_history(array, length, bincdata);
            g_task_run_in_thread(bincdata->task, save_history);
            create_chart(bincdata);
        }
    }
    else if (strcmp("ohlc", type) == 0)
    {
        update_candle(bincdata, json_object_get_object_member(object, "ohlc"));
    }
    else if (strcmp("time", type) == 0)
    {
        time_t raw_time;
        time(&raw_time);
        double time_difference = (raw_time - json_object_get_int_member(object, "time")) / 60;
        printf("Time Difference: %.2f\n", time_difference);
    }
    else if (strcmp("active_symbols", type) == 0)
    {
        update_active_symbols(bincdata, json_object_get_array_member(object, "active_symbols"));
        // setup_instrument(bincdata);
        /*if (window->model->instrument == NULL)
        {
            set_default_instrument(window->home);
            // window->model->instrument = restore_last_instrument(window->home);
        }*/
    }

    else
        g_print("Received message: %s\n", type);
    // g_date_time_unref(date);
}

static void on_message(SoupWebsocketConnection *connection, SoupWebsocketDataType type, GBytes *message, BincData *bincdata)
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
            handle_socket_response(object, type, bincdata);
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

void on_closed(SoupWebsocketConnection *connection, BincData *bincdata)
{
    const gchar *reason = soup_websocket_connection_get_close_data(connection);
    if (reason && g_str_equal(reason, "Symbol Change"))
    {
        GListModel *model = G_LIST_MODEL(bincdata->data->labels);
        for (gint index = 0; index < g_list_model_get_n_items(model); index++)
        {
           GtkLabel *label = g_list_model_get_item(model, index);
           gtk_label_set_text(label, "");
        }
        
        free_model(G_OBJECT(bincdata->data->box));
        free_model(G_OBJECT(bincdata->data->chart));
        free_candle_history(bincdata->store);
        g_task_run_in_thread(bincdata->task, setup_soup_session);
    }
    g_object_unref(connection);
    // gtk_toggle_button_set_active(bincdata->led, FALSE);
}

static void on_websocket_connected(SoupSession *session, GAsyncResult *result, BincData *bincdata)
{
    GError *error = NULL;

    bincdata->connection = soup_session_websocket_connect_finish(session, result, &error);

    if (error)
    {
        g_printerr("Failed to connect to WebSocket: %s\n", error->message);
        g_error_free(error);
        return;
    }

    // gtk_toggle_button_set_active(object->led, TRUE);

    g_signal_connect(bincdata->connection, "message", (GCallback)on_message, bincdata);
    g_signal_connect(bincdata->connection, "closed", (GCallback)on_closed, bincdata);

    if (bincdata->profile->token == NULL)
    {
        soup_websocket_connection_send_text(bincdata->connection, request_active_symbols());
    }
    else
    {
        restore_last_instrument(bincdata);
        gint candles = restore_candles(bincdata);

        if (candles > 0)
        {
            gchar *instruction = request_tick_history(bincdata->instrument->symbol, candles);
            soup_websocket_connection_send_text(bincdata->connection, instruction);
        }
    }
}

void setup_soup_session(GTask *task, gpointer source, gpointer data, GCancellable *unused)
{
    BincData *bincdata = (BincData *)data;
    SoupSession *session = soup_session_new();
    const char *uri = "wss://ws.derivws.com/websockets/v3?app_id=66477";
    SoupMessage *message = soup_message_new(SOUP_METHOD_GET, uri);
    if (bincdata->profile->token != NULL)
    {
        int maxlength = 8 + strlen(bincdata->profile->token);
        char *bearer = (char *)malloc(maxlength);
        snprintf(bearer, maxlength, "Bearer %s", bincdata->profile->token);
        SoupMessageHeaders *headers = soup_message_get_request_headers(message);
        soup_message_headers_append(headers, "Authorization", bearer);
        free(bearer);
    }
    soup_session_websocket_connect_async(
        session, message, NULL, NULL, G_PRIORITY_HIGH, NULL,
        (GAsyncReadyCallback)on_websocket_connected, bincdata);
    g_object_unref(message);
    g_object_unref(session);
}