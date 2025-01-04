#include "session.h"

void set_candle_data(Candle *candle)
{
    true;
}

void get_candle_history(JsonArray *array, guint length, BincWindow *window)
{
    double highest = 0;
    double lowest = 0;
    size_t skip = window->model->count - length;

    for (size_t index = 0; index < window->model->count; index++)
    {
        Candle *candle;
        if (index >= skip)
        {
            JsonObject *object = json_node_get_object(json_array_get_element(array, index - skip));
            gint64 time = json_object_get_int_member(object, "epoch");

            candle = g_object_new(CANDLE_TYPE_OBJECT, NULL);
            candle->data = window->model->data;
            candle->time = window->model->time;

            candle->price->close = json_object_get_double_member(object, "close");
            candle->price->epoch = g_date_time_new_from_unix_utc(time);
            candle->price->high = json_object_get_double_member(object, "high");
            candle->price->low = json_object_get_double_member(object, "low");
            candle->price->open = json_object_get_double_member(object, "open");
            json_object_unref(object);

            candle_list_model_add_item(window->model, candle);
            if (index == window->model->count - 1)
            {
                window->price = candle->price;
            }
        }
        else
            candle = g_list_model_get_item((GListModel *)window->model, index);

        if (highest < candle->price->high)
            highest = candle->price->high;
        if ((candle->price->low < lowest) || (lowest == 0))
            lowest = candle->price->low;

        if (index == window->model->count - 1)
        {
            highest = ceil(highest);
            lowest = floor(lowest);
            double range = highest - lowest;
            window->model->data->highest = highest;
            window->model->data->lowest = lowest;
            window->model->data->baseline = lowest + range / 2;
            window->model->data->scale = 2 / range;
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

Tick *get_tick(gchar *data, gssize size)
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
        Tick *tick = g_new0(Tick, 1);
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

void update_candle(BincWindow *window, JsonObject *object)
{
    gint64 time = json_object_get_int_member(object, "epoch");
    GDateTime *current_time = g_date_time_new_from_unix_utc(time);

    if (g_date_time_get_second(current_time) == 0)
    {
        Candle *candle = g_object_new(CANDLE_TYPE_OBJECT, NULL);
        candle->data = window->model->data;
        candle->time = window->model->time;
        char *endptr;
        candle->price->high = strtod(json_object_get_string_member(object, "high"), &endptr);
        candle->price->low = strtod(json_object_get_string_member(object, "low"), &endptr);
        candle->price->open = strtod(json_object_get_string_member(object, "open"), &endptr);
        candle->price->close = strtod(json_object_get_string_member(object, "close"), &endptr);
        candle->price->epoch = current_time;

        free(window->price);
        window->price = candle->price;
        window->widget = create_canvas(candle);
        gtk_box_append(window->chart, window->widget);
        gtk_range_set_value(window->scale, candle->price->close);
        gtk_viewport_scroll_to(window->timeport, (GtkWidget *)window->chart, window->scrollinfo);
    }
    else if (g_date_time_get_second(current_time) == 59)
    {
        CandleListModel *model = g_object_new(CANDLE_TYPE_LIST_MODEL, NULL);
        model->timeframe = window->model->timeframe;
        model->home = window->model->home;
        model->instrument = window->model->instrument;
        Candle *candle = g_object_new(CANDLE_TYPE_OBJECT, NULL);
        *candle->price = *window->price;
        candle_list_model_add_item(model, candle);
        pthread_t thread;
        pthread_create(&thread, NULL, save_history, (void *)model);
        pthread_detach(thread);
    }
    else if (window->widget != NULL && GTK_IS_WIDGET(window->widget))
    {
        char *endptr;
        window->price->high = strtod(json_object_get_string_member(object, "high"), &endptr);
        window->price->low = strtod(json_object_get_string_member(object, "low"), &endptr);
        window->price->open = strtod(json_object_get_string_member(object, "open"), &endptr);
        window->price->close = strtod(json_object_get_string_member(object, "close"), &endptr);
        window->price->epoch = current_time;
        gtk_range_set_value(window->scale, window->price->close);
        gtk_widget_queue_draw(window->widget);
    }
}

static GLuint compile_shader(GLenum type, const char *name)
{
    char *resourcePath = malloc(strlen(name) + 26);
    sprintf(resourcePath, "/com/binclab/terminal/gl/%s", name);
    GBytes *bytes = g_resources_lookup_data(resourcePath, 0, NULL);
    free(resourcePath);
    if (!bytes)
    {
        g_warning("ERROR::SHADER::RESOURCE_NOT_FOUND\n");
        return 0;
    }

    gsize size;
    const char *shaderCode = (const gchar *)g_bytes_get_data(bytes, &size);
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);
    g_bytes_unref(bytes);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        g_warning("ERROR::SHADER::COMPILATION_FAILED\n%s", infoLog);
        return 0;
    }
    return shader;
}

static void setup_instrument(DataObject *object)
{
    object->instrument = restore_last_instrument(object);
    if (object->instrument != NULL)
    {
        object->data->vertex = compile_shader(GL_VERTEX_SHADER, "shader.vert");
        object->data->fragment = compile_shader(GL_FRAGMENT_SHADER, "shader.frag");
        printf("dane\n");
    }
}

static void on_message(SoupWebsocketConnection *connection, SoupWebsocketDataType type, GBytes *message, DataObject *dataobject)
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

            GDateTime *date = g_date_time_new_now_utc();
            if (strcmp("candles", type) == 0)
            {
                JsonArray *array = json_object_get_array_member(object, "candles");
                guint length = json_array_get_length(array);
                pthread_t thread;
                // get_candle_history(array, length, object);
                // pthread_create(&thread, NULL, save_history, (void *)object);
                // pthread_detach(thread);
                // create_chart(window);
                //  g_print("Received message: %.*s\n", (int)size, data);
                /*gchar time[] = "xx:xx";
                gint hours = g_date_time_get_hour(candle->epoch);
                gint seconds = g_date_time_get_minute(candle->epoch);
                sprintf(time, "%i:%02i", hours, seconds + 1);*/
            }
            else if (strcmp("ohlc", type) == 0)
            {
                // update_candle(window, json_object_get_object_member(object, "ohlc"));
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
                update_active_symbols(dataobject, json_object_get_array_member(object, "active_symbols"));
                setup_instrument(dataobject);
                /*if (window->model->instrument == NULL)
                {
                    set_default_instrument(window->home);
                    // window->model->instrument = restore_last_instrument(window->home);
                }*/
            }

            else
                g_print("Received message: %s\n", type);
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

void on_closed(SoupWebsocketConnection *connection, DataObject *window)
{

    g_object_unref(connection);
    gtk_toggle_button_set_active(window->led, FALSE);
}

static void on_websocket_connected(SoupSession *session, GAsyncResult *result, DataObject *object)
{
    GError *error = NULL;

    object->connection = soup_session_websocket_connect_finish(session, result, &error);

    if (error)
    {
        g_printerr("Failed to connect to WebSocket: %s\n", error->message);
        g_error_free(error);
        return;
    }

    // gtk_toggle_button_set_active(object->led, TRUE);

    g_signal_connect(object->connection, "message", (GCallback)on_message, object);
    g_signal_connect(object->connection, "closed", (GCallback)on_closed, object);

    if (object->instrument == NULL)
    {
        setup_instrument(object);
    }

    /*
        GDateTime *timeutc = g_date_time_new_now_utc();

        int candles;
        if (object->widget == NULL)
        {
            candles = object->rectangle->width / CANDLE_SIZE;
            object->instrument = restore_last_instrument(object->home);
        }
        else
        {
            GTimeSpan span = g_date_time_difference(object->price->epoch, timeutc);
            candles = (int)(span / G_TIME_SPAN_MINUTE);
        }

        if (object->instrument == NULL)
        {
            soup_websocket_connection_send_text(object->connection, request_active_symbols());
            // TODO RE RUN LOGIC
        }
        else
        {
            //candles = restore_candles(object);

            if (candles > 0)
            {
                gchar *instruction = request_tick_history(object->instrument->symbol, candles);
                soup_websocket_connection_send_text(object->connection, instruction);
            }
        }
        */
}

void *setup_soup_session(void *argument)
{
    DataObject *object = (DataObject *)argument;
    SoupSession *session = soup_session_new();
    const char *uri = "wss://ws.derivws.com/websockets/v3?app_id=66477";
    SoupMessage *message = soup_message_new(SOUP_METHOD_GET, uri);
    SoupMessageHeaders *headers = soup_message_get_request_headers(message);
    soup_message_headers_append(headers, "Authorization", "Bearer 3PWlzpXRl8GcIeR");
    soup_session_websocket_connect_async(
        session, message, NULL, NULL, G_PRIORITY_HIGH, NULL,
        (GAsyncReadyCallback)on_websocket_connected, object);
    g_object_unref(message);
    g_object_unref(session);
    return NULL;
}