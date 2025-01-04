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

static GtkWidget *layout1, *layout2, *canvasbox, *layout3, *layout4, *fixed;
static GtkWidget *scaleinfo, *chart, *timechart, *header1, *infoarea;
static GtkWidget *navigation, *terminfo, *termframe, *infobox, *scale;
static GtkWidget *accountbook, *termbar, *termspace, *conled;
static GtkWidget *scaleport, *scalescroll, *scalebox; //, *chartpane, *timepane;
static GtkWidget *chartscroll, *timescroll, *chartport, *timeport;

typedef struct
{
    double space;
    int height;
} Increment;

GtkIconTheme *theme;

#endif // MAIN_H