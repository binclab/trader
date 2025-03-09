#ifndef CLEANUP_H
#define CLEANUP_H

#include <gtk/gtk.h>

void shutdown(GtkApplication *application, gpointer userdata);
void free_instrument(GObject *symbol);
void free_candles(GListStore *store);

#endif // CLEANUP_H