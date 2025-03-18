#include "chart.h"

void add_widgets(GObject *source, GAsyncResult *result, gpointer userdata)
{
    GTask *task = G_TASK(result);
    if (g_task_propagate_boolean(task, NULL))
    {
        GObject *object = G_OBJECT(task);
        GtkWidget *area = GTK_WIDGET(g_object_get_data(object, "area"));
        GtkWidget *label = GTK_WIDGET(g_object_get_data(object, "label"));
        gint position = GPOINTER_TO_INT(g_object_get_data(object, "position"));

        GtkFixed *fixed = GTK_FIXED(g_object_get_data(source, "timefixed"));
        gtk_fixed_put(fixed, label, position, 0);
        fixed = GTK_FIXED(g_object_get_data(source, "chartfixed"));
        gtk_fixed_put(fixed, area, position, 0);
    }
}

void add_candle(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    GObject *object = G_OBJECT(source), *candle = G_OBJECT(userdata);

    GtkWidget *label = gtk_label_new(NULL);
    GtkWidget *widget = gtk_gl_area_new();
    GDateTime *utctime = (GDateTime *)g_object_get_data(candle, "epoch");
    GDateTime *localtime = g_date_time_to_local(utctime);
    gint hours = g_date_time_get_hour(localtime);
    gint minutes = g_date_time_get_minute(localtime);
    g_date_time_unref(localtime);
    gboolean even = minutes % 5 == 0;
    if (even)
    {
        gchar *name = (gchar *)g_malloc0(6);
        g_snprintf(name, 6, "%02i:%02i", hours, minutes);
        gtk_label_set_label(GTK_LABEL(label), name);
        g_clear_pointer(&name, g_free);
    }

    GtkGLArea *area = GTK_GL_AREA(widget);
    gtk_gl_area_set_has_stencil_buffer(area, TRUE);
    gtk_widget_set_vexpand(widget, FALSE);
    gtk_widget_set_size_request(widget, 24, 720);
    gtk_widget_set_size_request(label, 24, 48);
    gtk_widget_set_name(widget, "candle");
    gtk_widget_set_name(label, "time");

    GObject *fobject = G_OBJECT(g_object_get_data(object, "timefixed"));
    GListStore *store = G_LIST_STORE(g_object_get_data(fobject, "store"));

    int position = GPOINTER_TO_INT(g_object_get_data(fobject, "position")) + 24;
    GObject *tobject = G_OBJECT(task);
    g_object_set_data(fobject, "position", GINT_TO_POINTER(position));
    g_object_set_data(tobject, "position", GINT_TO_POINTER(position));
    g_object_set_data(tobject, "area", area);
    g_object_set_data(tobject, "label", label);
    g_list_store_append(store, label);

    fobject = G_OBJECT(g_object_get_data(object, "chartfixed"));
    store = G_LIST_STORE(g_object_get_data(fobject, "store"));
    position = GPOINTER_TO_INT(g_object_get_data(fobject, "position")) + 24;
    g_object_set_data(fobject, "position", GINT_TO_POINTER(position));
    g_list_store_append(store, widget);
    g_task_return_boolean(task, TRUE);
}