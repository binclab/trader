#include "chart.h"

void add_widgets(GObject *source, GAsyncResult *result, gpointer userdata)
{
    GObject *object = g_object_get_data(source, "timefixed");
    GListModel *timemodel = G_LIST_MODEL(g_object_get_data(object, "store"));
    GtkFixed *time = GTK_FIXED(object);
    object = g_object_get_data(source, "chartfixed");
    GListModel *chartmodel = G_LIST_MODEL(g_object_get_data(object, "store"));
    GtkFixed *chart = GTK_FIXED(object);
    for (gint index = 0; index < g_list_model_get_n_items(chartmodel); index++)
    {
        gint position = index * 24;
        GtkWidget *area = g_list_model_get_item(chartmodel, index);
        GtkWidget *label = g_list_model_get_item(timemodel, index);
        gpointer pointer = g_object_get_data(G_OBJECT(area), "height");
        gtk_fixed_put(time, label, position, 0);
        gtk_fixed_put(chart, area, position, GPOINTER_TO_INT(pointer));
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
    if (minutes % 5 == 0)
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

    GObject *tobject = G_OBJECT(task);
    g_object_set_data(tobject, "area", area);
    g_object_set_data(tobject, "label", label);
    g_list_store_append(store, label);

    fobject = G_OBJECT(g_object_get_data(object, "chartfixed"));
    store = G_LIST_STORE(g_object_get_data(fobject, "store"));
    g_list_store_append(store, widget);
}