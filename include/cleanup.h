#ifndef CLEANUP_H
#define CLEANUP_H

#include <gtk/gtk.h>
#include <GLES3/gl3.h>

void shutdown(GtkApplication *application, gpointer userdata);
void free_instrument(GObject *symbol);
void free_list(GTask *task, gpointer source, gpointer userdata, GCancellable *unused);
gboolean close_window(GtkWindow *window, gpointer userdata);
void unrealize_cartesian(GtkGLArea *area, gpointer userdata);

#endif // CLEANUP_H