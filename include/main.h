#ifndef MAIN_H
#define MAIN_H

#include "session.h"
#include "window.h"

#include <libsecret/secret.h>

GApplication *gapp;
GNetworkMonitor *networkmonitor;
GdkDisplay *display;
GdkMonitor *projector;
GdkRectangle geometry;
GdkSurface *surface;
GtkStyleProvider *provider;
GtkStyleContext *context;

typedef struct
{
    double space;
    int height;
} Increment;

GtkIconTheme *theme;

#endif // MAIN_H