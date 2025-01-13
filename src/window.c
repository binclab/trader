#include "window.h"

G_DEFINE_TYPE(BincWindow, binc_window, GTK_TYPE_APPLICATION_WINDOW)

static void binc_window_size_allocate(GtkWidget *window, int width, int height, int baseline)
{
    ((GtkWidgetClass *)binc_window_parent_class)->size_allocate(window, width, height, baseline);
    if (((BincWindow *)window)->set && gtk_window_is_maximized((GtkWindow *)window))
    {
        ((BincWindow *)window)->width = width;
        ((BincWindow *)window)->height = height;
        ((BincWindow *)window)->set = FALSE;
        gtk_window_close((GtkWindow *)window);
    }
}

static void binc_window_class_init(BincWindowClass *class)
{
    ((GtkWidgetClass *)class)->size_allocate = binc_window_size_allocate;
}

static void binc_window_init(BincWindow *self)
{
    self->set = TRUE;
    gtk_window_set_decorated((GtkWindow *)self, FALSE);
    gtk_widget_remove_css_class((GtkWidget *)self, "background");
}

GtkWidget *binc_window_new(GtkApplication *app)
{
    return g_object_new(BINC_TYPE_WINDOW, "application", app, NULL);
}