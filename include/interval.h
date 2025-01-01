#ifndef INTERVAL_H
#define INTERVAL_H

#include <gtk/gtk.h>

#define BINC_TYPE_INTERVAL binc_interval_get_type()

#define BINC_TYPE_CURRENT binc_current_get_type()

typedef struct
{
    GtkGLArea parent_instance;
} BincInterval;

typedef struct
{
    GtkGLAreaClass parent_class;
} BincIntervalClass;

typedef struct
{
    float x1;
    float y1;
    float x2;
    float y2;
} CanvasVertices;

//GtkWidget *binc_interval_new(CandleInfo *value);

#endif // INTERVAL_H