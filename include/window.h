#ifndef BINC_WINDOW_H
#define BINC_WINDOW_H

#include <gtk/gtk.h>

#define BINC_TYPE_WINDOW binc_window_get_type()

typedef struct
{
    GtkApplicationWindow parent_instance;
    gboolean set;
    int width;
    int height;
} BincWindow;

typedef struct
{
    GtkApplicationWindowClass parent_class;
} BincWindowClass;

GtkWidget *binc_window_new(GtkApplication *app);

#endif // BINC_WINDOW_H