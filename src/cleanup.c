#include "cleanup.h"

static void free_tasks(GTask *task, gpointer source, gpointer data, GCancellable *unused)
{
    BincData *bincdata = (BincData *)data;
    g_object_unref(task);
    g_object_unref(bincdata->task);
    free(bincdata);
}

void free_instrument(BincSymbol *instrument)
{
    g_clear_pointer(&instrument->spot_age, g_free);
    g_clear_pointer(&instrument->spot_percentage_change, g_free);
    g_clear_pointer(&instrument->spot_time, g_free);
    g_clear_pointer(&instrument->subgroup, g_free);
    g_clear_pointer(&instrument->subgroup_display_name, g_free);
    g_clear_pointer(&instrument->submarket, g_free);
    g_clear_pointer(&instrument->submarket_display_name, g_free);
    g_clear_pointer(&instrument->symbol, g_free);
    g_clear_pointer(&instrument->symbol_type, g_free);

    g_free(instrument);
    instrument = NULL;
}

static void free_favourite_list(GListModel *model)
{
    GListStore *store = G_LIST_STORE(model);

    for (guint index = 0; index < g_list_model_get_n_items(model); index++)
    {
        gpointer item = g_list_model_get_item(model, index);
        if (item != NULL)
        {
            g_object_unref(item);
        }
    }
    g_list_store_remove_all(store);
    g_object_unref(model);
}

void free_model(GObject *object)
{
    GListModel *model = G_LIST_MODEL(g_object_get_data(object, "model"));

    for (guint index = 0; index < g_list_model_get_n_items(model); index++)
    {
        GtkWidget *child = g_list_model_get_item(model, index);
        gtk_box_remove(GTK_BOX(object), child);
    }
    g_list_store_remove_all(G_LIST_STORE(model));
}

void free_candle_history(GListStore *store)
{
    GListModel *model = G_LIST_MODEL(store);

    for (guint index = 0; index < g_list_model_get_n_items(model); index++)
    {
        gpointer item = g_list_model_get_item(model, index);
        if (item != NULL)
        {
            g_object_unref(item);
        }
    }
    g_list_store_remove_all(store);
}

void shutdown(GtkApplication *application, BincData *bincdata)
{
    g_free(bincdata->account[0]->token);
    g_free(bincdata->account[1]->token);
    g_free(bincdata->account[2]->token);
    free(bincdata->account[0]);
    free(bincdata->account[1]);
    free(bincdata->account[2]);

    free_instrument(bincdata->instrument);
    free_favourite_list(gtk_drop_down_get_model(bincdata->symbolbox));
    free(bincdata->time);
    free(bincdata->data);
    free(bincdata->rectangle);
    g_object_unref(bincdata->store);
    free(bincdata->home);
    g_object_unref(bincdata->connection);
    g_object_unref(bincdata->task);
    // free(bincdata);

    g_object_unref(bincdata);
    g_object_unref(application);
}

void close_request(GtkWindow *window, BincData *bincdata)
{
    glDeleteProgram(bincdata->data->program);
    gtk_window_destroy(bincdata->window);
}