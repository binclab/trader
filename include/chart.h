#ifndef CHART_H
#define CHART_H

#include <gtk/gtk.h>
#define CANDLE_WIDTH 32

void add_candle_to_chart(gpointer data, gpointer userdata);
void add_candle(GObject *object, GObject *candle);
gboolean add_widgets(gpointer userdata);

#endif // CHART_H