#ifndef CLEANUP_H
#define CLEANUP_H
#include "symbols.h"

static void free_tasks(GTask *task, gpointer source, gpointer data, GCancellable *unused)
{
    BincData *object = (BincData *)data;
    g_object_unref(task);
    g_object_unref(object->task);
    free(object);
}

static void shutdown(GtkApplication *application, BincData *object)
{
    g_free(object->account[0]->token);
    g_free(object->account[1]->token);
    g_free(object->account[2]->token);
    free(object->account[0]);
    free(object->account[1]);
    free(object->account[2]);

    g_free(object->instrument->spot_age);
    g_free(object->instrument->spot_percentage_change);
    g_free(object->instrument->spot_time);
    g_free(object->instrument->subgroup);
    g_free(object->instrument->subgroup_display_name);
    g_free(object->instrument->submarket);
    g_free(object->instrument->submarket_display_name);
    g_free(object->instrument->symbol);
    g_free(object->instrument->symbol_type);

    free(object->instrument);

    free(object->time);
    free(object->data);
    free(object->rectangle);
    g_object_unref(object->store);
    free(object->home);
    g_object_unref(object->connection);
    g_object_unref(object->task);
    //free(object);
}

static void close_request(GtkWindow *window, BincData *object)
{
    glDeleteProgram(object->data->program);
    gtk_window_destroy(object->window);
}

#endif // CLEANUP_H