#ifndef MAIN_H
#define MAIN_H

#include "canvas.h"
#include "session.h"
#include "window.h"

GApplication *gapp;
GNetworkMonitor *networkmonitor;
GdkDisplay *display;
GdkMonitor *projector;
GdkRectangle geometry;
GdkSurface *surface;
GtkStyleProvider *provider;
GtkStyleContext *context;

static GtkWidget *layout1, *layout2, *canvasbox, *layout4;
static GtkWidget *scaleinfo, *chart, *timechart, *header1, *infoarea;
static GtkWidget *navigation, *scale, *terminfo, *termframe, *infobox;
static GtkWidget *accountbook, *termbar, *termspace, *conled;
static GtkWidget *scaleport, *scalescroll, *scalebox;
static GtkWidget *chartscroll, *timescroll, *chartport, *timeport;

typedef struct
{
    double space;
    int height;
} Increment;

GtkIconTheme *theme;

#endif // MAIN_H