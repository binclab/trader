#include "cleanup.h"

void shutdown(GtkApplication *application, gpointer userdata)
{
    GObject *object = G_OBJECT(userdata);
    // Checking if pointer is not null and unrefencing
    gpointer pointer = g_object_get_data(object, "connection");
    if (pointer)
    {
        g_object_unref(pointer);
    }

    pointer = g_object_get_data(object, "session");
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
        g_free(pointer);
    }

    pointer = g_object_get_data(object, "candles");
    if (pointer)
    {
        free_candles(G_LIST_STORE(pointer));
        g_object_unref(pointer);
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
    g_object_set_data(object, "price", NULL);
    g_object_set_data(object, "epoch", NULL);
    g_object_set_data(object, "home", NULL);
    g_object_set_data(object, "token", NULL);
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

void free_candles(GListStore *store)
{
    GListModel *model = G_LIST_MODEL(store);
    gint count = g_list_model_get_n_items(model);
    for (gint index = 0; index < count; index++)
    {
        GObject *object = g_list_model_get_item(model, index);
        gpointer pointer = g_object_get_data(object, "price");
        g_free(pointer);
        pointer = g_object_get_data(object, "epoch");
        if (pointer)
        {
            g_date_time_unref((GDateTime *)pointer);
        }
    }
}