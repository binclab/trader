#include "window.h"

G_DEFINE_TYPE(BincWindow, binc_window, GTK_TYPE_APPLICATION_WINDOW)

static void binc_window_size_allocate(GtkWidget *window, int width, int height, int baseline)
{
    ((GtkWidgetClass *)binc_window_parent_class)->size_allocate(window, width, height, baseline);
    if (((BincWindow *)window)->set && gtk_window_is_maximized((GtkWindow *)window))
    {
        ((BincWindow *)window)->width = width - CONTENT_WIDTH;
        ((BincWindow *)window)->height = height - CONTENT_HEIGHT;
        ((BincWindow *)window)->set = FALSE;
    }
}

static gboolean close_window(GtkWindow *widget)
{
    BincWindow *window = (BincWindow *)widget;
    gtk_window_destroy((GtkWindow *)window);
    return TRUE;
}

static void binc_window_class_init(BincWindowClass *klass)
{
    ((GtkWidgetClass *)klass)->size_allocate = binc_window_size_allocate;
    ((GtkWindowClass *)klass)->close_request = close_window;
}

static void binc_window_init(BincWindow *self)
{
    GtkWindow *window = (GtkWindow *)self;
    GDateTime *timelocal = g_date_time_new_now_local();
    GDateTime *timeutc = g_date_time_new_now_utc();
    self->set = TRUE;
    self->print = FALSE;
    self->bytes_recieved = 0;
    self->width = 1280;
    self->height = 720;
    self->model = g_object_new(CANDLE_TYPE_LIST_MODEL, NULL);
    self->model->time->hours = g_date_time_get_hour(timelocal) - g_date_time_get_hour(timeutc);
    self->model->time->minutes = g_date_time_get_minute(timelocal) - g_date_time_get_minute(timeutc);

    gtk_window_set_default_size((GtkWindow *)self, 1280, 720);
    //gtk_window_set_resizable((GtkWindow *)self, FALSE);
    //gtk_window_set_focus_visible((GtkWindow *)self, TRUE);
}

GtkWidget *binc_window_new(GtkApplication *app)
{
    return g_object_new(BINC_TYPE_WINDOW, "application", app, NULL);
}