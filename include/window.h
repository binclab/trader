#ifndef BINC_WINDOW_H
#define BINC_WINDOW_H

#include "canvas.h"
#include <gtk/gtk.h>

#define BINC_TYPE_WINDOW binc_window_get_type()

typedef struct
{
    GtkApplicationWindow parent_instance;
    gboolean set;
    gboolean print;
    int width;
    int height;
    GtkWidget *widget;
    CandleListModel *model;
    CandlePrice *price;
    char *timeframe;
    GtkScrollInfo *scrollinfo;
    GtkViewport *timeport;
    GtkToggleButton *led;
    GtkRange *scale;
    GtkBox *scalebox;
    GtkBox *chart;
    gsize bytes_recieved;
    GtkLabel *infolabel;
    SoupWebsocketConnection *connection;
    gchar *home;
} BincWindow;

typedef struct
{
    GtkApplicationWindowClass parent_class;
} BincWindowClass;

GtkWidget *binc_window_new(GtkApplication *app);

#endif // BINC_WINDOW_H