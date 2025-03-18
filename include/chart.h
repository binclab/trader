#ifndef CHART_H
#define CHART_H

#include <gtk/gtk.h>

void add_candle_to_chart(gpointer data, gpointer userdata);
void add_candle(GTask *task, gpointer source, gpointer userdata, GCancellable *unused);
void add_widgets(GObject *source, GAsyncResult *result, gpointer userdata);

#endif // CHART_H