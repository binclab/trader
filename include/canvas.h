#ifndef CANVAS_H_INCLUDED
#define CANVAS_H_INCLUDED

#include "symbols.h"
#include <epoxy/gl.h>

#define CANDLE_SIZE 50
#define CONTENT_HEIGHT 300
#define CONTENT_WIDTH 302
#define THIN_LINE 0.075f
#define THICK_LINE 0.4f

#define CANDLE_TYPE_OBJECT (candle_object_get_type())
G_DECLARE_FINAL_TYPE(Candle, candle_object, CANDLE, OBJECT, GObject)

#define CANDLE_TYPE_LIST_MODEL (candle_list_model_get_type())
G_DECLARE_FINAL_TYPE(CandleListModel, candle_list_model, CANDLE, LIST_MODEL, GObject)

typedef struct
{
    double ask;
    double bid;
    gint64 epoch;
    const gchar *id;
    gint pip_size;
    double quote;
    const gchar *symbol;
} Tick;

typedef enum
{
    TICK,
    CANDLES
} TickType;

typedef struct
{
    guint minutes;
    guint hours;
} CandleTime;

typedef struct
{
    GtkBox *box;
    GLuint program;
    GLuint buffer;
    GLuint vertex;
    GLuint fragment;
    double baseline;
    double scale;
    double highest;
    double lowest;
    int height;
    double space;
} CandleData;

typedef struct
{
    double high;
    double low;
    double open;
    double close;
    GDateTime *epoch;
} CandlePrice;

typedef struct _Candle
{
    GObject parent_instance;
    CandlePrice *price;
    CandleData *data;
    CandleTime *time;
} Candle;

typedef struct _CandleListModel
{
    GObject parent_instance;
    GPtrArray *candles;
    CandleData *data;
    CandleTime *time;
    char *home;
    char *timeframe;
    int count;
    Symbol *instrument;
} CandleListModel;

typedef struct
{
    char *home;
    GdkRectangle *rectangle;
    GListStore *store;
    Symbol *instrument;

    SoupWebsocketConnection *connection;

    GtkToggleButton *led;
    GtkWidget *widget;

    CandleData *data;
    CandlePrice *price;
    CandleTime *time;
} DataObject;

void candle_list_model_add_item(CandleListModel *model, Candle *candle);
GtkWidget *create_canvas(Candle *candle);
GtkWidget *create_scale(double price);

#endif // CANVAS_H_INCLUDED
