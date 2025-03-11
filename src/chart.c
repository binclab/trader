#include "chart.h"

void add_candle_to_chart(gpointer data, gpointer userdata)
{
    GObject *object = G_OBJECT(userdata), *candle = G_OBJECT(data);

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

    GtkFixed *fixed = GTK_FIXED(g_object_get_data(object, "timefixed"));

    GObject *fobject = G_OBJECT(fixed);
    GListStore *store = G_LIST_STORE(g_object_get_data(fobject, "store"));

    int position = GPOINTER_TO_INT(g_object_get_data(fobject, "position")) + 24;
    gtk_fixed_put(fixed, label, position, 0);
    g_object_set_data(fobject, "position", GINT_TO_POINTER(position));
    g_list_store_append(store, label);

    fixed = GTK_FIXED(g_object_get_data(object, "chartfixed"));
    fobject = G_OBJECT(fixed);
    store = G_LIST_STORE(g_object_get_data(fobject, "store"));
    position = GPOINTER_TO_INT(g_object_get_data(fobject, "position")) + 24;
    gtk_fixed_put(fixed, widget, position, 0);
    g_object_set_data(fobject, "position", GINT_TO_POINTER(position));
    g_list_store_append(store, widget);
}