#include "cleanup.h"

static void free_pointer(GObject *source, GAsyncResult *result, gpointer userdata)
{
    g_object_unref(userdata);
}

static void destroy_widgets(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    GObject *object = G_OBJECT(source);
    gpointer pointer[2] = {g_object_get_data(object, "chartfixed"),
                           g_object_get_data(object, "timefixed")};
    for (gint index = 0; index < 2; index++)
    {
        if (pointer[index])
        {
            gpointer store = g_object_get_data(G_OBJECT(pointer[index]), "store");
            if (store)
            {
                GTask *task = g_task_new(store, NULL, free_pointer, store);
                g_task_set_task_data(task, store, NULL);
                g_task_run_in_thread(task, free_list);
                g_object_unref(task);
            }
        }
    }
}

static void remove_data(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    GObject *object = G_OBJECT(source);
    gpointer pointer = g_object_get_data(object, "session");
    if (pointer)
    {
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "message");
    if (pointer)
    {
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "time");
    if (pointer)
    {
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "stat");
    if (pointer)
    {
        gpointer data = g_object_get_data(pointer, "highest");
        if (data)
            g_free(data);
        data = g_object_get_data(pointer, "lowest");
        if (data)
            g_free(data);
        data = g_object_get_data(pointer, "range");
        if (data)
            g_free(data);
        data = g_object_get_data(pointer, "baseline");
        if (data)
            g_free(data);
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "pip");
    if (pointer)
    {
        gpointer data = g_object_get_data(pointer, "highest");
        if (data)
            g_free(data);
        data = g_object_get_data(pointer, "lowest");
        if (data)
            g_free(data);
        data = g_object_get_data(pointer, "range");
        if (data)
            g_free(data);
        data = g_object_get_data(pointer, "midpoint");
        if (data)
            g_free(data);
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "candles");
    if (pointer)
    {
        GTask *task = g_task_new(object, NULL, free_pointer, pointer);
        g_task_set_task_data(task, G_LIST_MODEL(pointer), NULL);
        g_task_run_in_thread(task, free_list);
        g_object_unref(task);
    }

    pointer = g_object_get_data(object, "profile");
    if (pointer)
    {
        GListModel *model = G_LIST_MODEL(pointer);
        gint count = g_list_model_get_n_items(model);
        for (gint index = 0; index < count; index++)
        {
            GObject *object = g_list_model_get_item(model, index);
            gchar *data = g_object_get_data(object, "currency");
            g_clear_pointer(&data, g_free);
            data = g_object_get_data(object, "token");
            g_clear_pointer(&data, g_free);
        }
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "epoch");
    if (pointer)
    {
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "token");
    if (pointer)
    {
        g_object_unref(pointer);
    }

    // Freeing strings and other dynamically allocated memory

    pointer = g_object_get_data(object, "home");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "display_name");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "exchange_name");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "market_display_name");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "market");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "quoted_currency_symbol");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "spot_age");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "spot_percentage_change");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "spot_time");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "subgroup_display_name");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "subgroup");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "submarket_display_name");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "submarket");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    pointer = g_object_get_data(object, "symbol_type");
    if (pointer)
    {
        g_clear_pointer(&pointer, g_free);
    }

    // Clearing the data pointers
    g_object_set_data(object, "display_name", NULL);
    g_object_set_data(object, "exchange_name", NULL);
    g_object_set_data(object, "market_display_name", NULL);
    g_object_set_data(object, "market", NULL);
    g_object_set_data(object, "quoted_currency_symbol", NULL);
    g_object_set_data(object, "spot_age", NULL);
    g_object_set_data(object, "spot_percentage_change", NULL);
    g_object_set_data(object, "spot_time", NULL);
    g_object_set_data(object, "subgroup_display_name", NULL);
    g_object_set_data(object, "subgroup", NULL);
    g_object_set_data(object, "submarket_display_name", NULL);
    g_object_set_data(object, "submarket", NULL);
    g_object_set_data(object, "symbol_type", NULL);

    g_object_set_data(object, "time", NULL);
    g_object_set_data(object, "stat", NULL);
    g_object_set_data(object, "data", NULL);
    g_object_set_data(object, "epoch", NULL);
    g_object_set_data(object, "home", NULL);
    g_object_set_data(object, "token", NULL);
}

void shutdown(GtkApplication *application, gpointer userdata)
{
    GTask *task = g_task_new(userdata, NULL, NULL, NULL);
    g_task_run_in_thread(task, remove_data);
    g_object_unref(task);
}

void free_instrument(GObject *symbol)
{
    gpointer pointer = g_object_get_data(symbol, "spot_age");
    g_clear_pointer(&pointer, g_free);
    pointer = g_object_get_data(symbol, "spot_percentage_change");
    g_clear_pointer(&pointer, g_free);

    pointer = g_object_get_data(symbol, "spot_time");
    g_clear_pointer(&pointer, g_free);

    pointer = g_object_get_data(symbol, "subgroup");
    g_clear_pointer(&pointer, g_free);

    pointer = g_object_get_data(symbol, "subgroup_display_name");
    g_clear_pointer(&pointer, g_free);

    pointer = g_object_get_data(symbol, "submarket");
    g_clear_pointer(&pointer, g_free);

    pointer = g_object_get_data(symbol, "submarket_display_name");
    g_clear_pointer(&pointer, g_free);

    pointer = g_object_get_data(symbol, "symbol_type");
    g_clear_pointer(&pointer, g_free);

    pointer = g_object_get_data(symbol, "spot");
    g_free(pointer);

    g_free(symbol);
}

void free_list(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    GListModel *model = G_LIST_MODEL(userdata);

    if (G_IS_LIST_MODEL(model))
    {
        gint count = g_list_model_get_n_items(model);

        for (gint index = 0; index < count; index++)
        {
            GObject *item = g_list_model_get_item(model, index);
            if (item)
            {
                gpointer pointer = g_object_get_data(item, "open");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "close");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "high");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "low");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "epoch");
                if (pointer)
                    g_date_time_unref((GDateTime *)pointer);
                pointer = g_object_get_data(item, "openPip");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "closePip");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "highPip");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "lowPip");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "midPip");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "rangePip");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "factor");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "ordinate");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "red");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "green");
                if (pointer)
                    g_free(pointer);
                pointer = g_object_get_data(item, "blue");
                if (pointer)
                    g_free(pointer);
                g_object_unref(item);
            }
        }
    }
}

void unrealize_cartesian(GtkGLArea *area, gpointer userdata)
{
    if (gtk_gl_area_get_error(area) == NULL)
    {
        GObject *object = G_OBJECT(area);
        GObject *data = G_OBJECT(g_object_get_data(G_OBJECT(userdata), "data"));
        GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(object, "buffer"));
        GLuint program = GPOINTER_TO_UINT(g_object_get_data(data, "program"));
        gpointer pointer = g_object_get_data(object, "store");
        GTask *task = g_task_new(NULL, NULL, free_pointer, pointer);
        g_task_set_task_data(task, pointer, NULL);
        g_task_run_in_thread(task, free_list);
        g_object_unref(task);
        glDeleteProgram(program);
        glDeleteVertexArrays(1, &buffer);
    }
}

gboolean close_window(GtkWindow *window, gpointer userdata)
{
    gtk_window_minimize(window);

    GTask *task = g_task_new(userdata, NULL, NULL, NULL);
    g_task_run_in_thread(task, destroy_widgets);
    g_object_unref(task);
    gtk_window_destroy(window);
    return true;
}