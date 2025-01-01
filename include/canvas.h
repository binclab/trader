#ifndef CANVAS_H_INCLUDED
#define CANVAS_H_INCLUDED

#include "symbols.h"
#include <epoxy/gl.h>

#define CANDLE_SIZE 50
#define CONTENT_HEIGHT 300
#define CONTENT_WIDTH 308
#define THIN_LINE 0.075f
#define THICK_LINE 0.4f

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

typedef struct {
    guint minutes;
    guint hours;
} CandleTime;

typedef struct {
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

typedef struct {
    double high;
    double low;
    double open;
    double close;
    GDateTime *epoch;
} CandlePrice;

typedef struct {
    CandlePrice *price;
    CandleData *data;
    CandleTime *time;
} Candle;

typedef struct {
    GObject parent_instance;
    GPtrArray *array;
    CandleData *data;
    CandleTime *time;
    char *home;
    char *timeframe;
    int count;
    Symbol *instrument;
} CandleListModel;

typedef struct {
    GObjectClass parent_class;
} CandleListModelClass;

void candle_list_model_add_item(CandleListModel *model, Candle *candle);

GtkWidget *create_canvas(Candle *candle);

#endif // CANVAS_H_INCLUDED