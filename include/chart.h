#ifndef CHART_H
#define CHART_H

#include "cleanup.h"
#define CANDLE_WIDTH 8
#define CANDLE_HEIGHT 24

void add_candle_to_chart(gpointer data, gpointer userdata);
void add_candle(GObject *object, GObject *candle);
void que_widgets(GObject *source, GAsyncResult *result, gpointer userdata);
gboolean add_widgets(gpointer userdata);

#endif // CHART_H