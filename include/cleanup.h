#ifndef CLEANUP_H
#define CLEANUP_H
#include "symbols.h"

void free_instrument(BincSymbol *instrument);
void close_request(GtkWindow *window, BincData *object);
void shutdown(GtkApplication *application, BincData *bincdata);
void free_model(GObject *object);
void free_candle_history(GListStore *store);

#endif // CLEANUP_H