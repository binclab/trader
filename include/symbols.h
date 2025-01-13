#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <webkit/webkit.h>
#include <sqlite3.h>

// #include <epoxy/gl.h>
#include <GLES3/gl3.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

#define SYMBOL_BOOM300 "BOOM300N"
#define SYMBOL_BOOM500 "BOOM500"
#define SYMBOL_BOOM1000 "BOOM1000"
#define SYMBOL_CRASH300 "CRASH300N"
#define SYMBOL_CRASH500 "CRASH500"
#define SYMBOL_CRASH1000 "CRASH1000"
#define SYMBOL_VI75 "R_75"
#define SYMBOL_GBPUSD "frxGBPUSD"
#define SYMBOL_GOLDBASKET "WLDXAU"

#define CANDLE_WIDTH 24
#define CANDLE_HEIGHT 720
#define MARKER_HEIGHT 36
#define CONTENT_HEIGHT 248
#define CONTENT_WIDTH 338
#define THIN_LINE 0.075f
#define THICK_LINE 0.6f

#define BINC_TYPE_CANDLE (binc_candle_get_type())
G_DECLARE_FINAL_TYPE(BincCandle, binc_candle, BINC, CANDLE, GObject)

#define BINC_TYPE_DATA (binc_data_get_type())
G_DECLARE_FINAL_TYPE(BincData, binc_data, BINC, DATA, GObject)

typedef struct _BincSymbol
{
    gint allow_forward_starting;
    gint delay_amount;
    gchar *display_name;
    gint display_order;
    gint exchange_is_open;
    gchar *exchange_name;
    gint intraday_interval_minutes;
    gint is_trading_suspended;
    gchar *market;
    gchar *market_display_name;
    gdouble pip;
    gchar *quoted_currency_symbol;
    gdouble spot;
    gchar *spot_age;
    gchar *spot_percentage_change;
    gchar *spot_time;
    gchar *subgroup;
    gchar *subgroup_display_name;
    gchar *submarket;
    gchar *submarket_display_name;
    gchar *symbol;
    gchar *symbol_type;
} BincSymbol;

typedef struct _AccountProfile
{
    gchar *account;
    gchar *token;
    gchar *currency;
} AccountProfile;

typedef struct _BincTick
{
    gdouble ask;
    gdouble bid;
    gint64 epoch;
    const gchar *id;
    gint pip_size;
    gdouble quote;
    const gchar *symbol;
} BincTick;

typedef enum
{
    TICK,
    CANDLES
} TickType;

typedef struct _CandleBuffer{
    GLuint vbo;
} CandleBuffer;

typedef struct _CandleTime
{
    guint minutes;
    guint hours;
} CandleTime;

typedef struct _CandleStatistics
{
    gdouble baseline;
    gdouble scale;
    gdouble highest;
    gdouble lowest;
    gdouble range;
} CandleStatistics;

typedef struct _CandleData
{

    GtkPaned *paned;
    GtkBox *box;
    GtkBox *chart;
    GtkOverlay *overlay;
    GtkBox *chartgrow;
    GtkBox *scalegrow;
    GtkRange *rangescale;
    GLuint program;
    gint marker;
    gdouble maximum;
    gdouble minimum;
    gdouble space;
    gdouble ordinate;
    gdouble close;
    gint abscissa;
    gint count;
} CandleData;

typedef struct _CandlePrice
{
    gdouble high;
    gdouble low;
    gdouble open;
    gdouble close;
    GDateTime *epoch;
} CandlePrice;

typedef struct _BincCandle
{
    GObject parent_instance;
    CandlePrice *price;
    CandleData *data;
    CandleTime *time;
    CandleStatistics *stat;
} BincCandle;

typedef struct _BincData
{
    GObject parent_instance;
    gchar *home;
    GdkRectangle *rectangle;
    GListStore *store;
    GTask *task;
    BincSymbol *instrument;

    SoupWebsocketConnection *connection;

    GtkToggleButton *led;
    GtkWidget *widget;
    GtkWidget *webview;
    GtkDrawingArea *pointer;
    GtkGLArea *cartesian;
    GtkWidget *child;
    GtkWindow *window;
    GtkWidget *chartgravity;
    GtkWidget *timegravity;
    GtkLabel *scalelabel;
    GtkViewport *timeport;
    GtkScrolledWindow *scalescroll;
    GtkScrollInfo *scrollinfo;
    GtkAdjustment *adjustment;

    AccountProfile *account[3];
    AccountProfile *profile;

    gchar *timeframe;
    gint count;

    CandleData *data;
    CandlePrice *price;
    CandleTime *time;
    CandleStatistics *stat;
} BincData;

GtkFixed *add_candle(BincData *bincdata, BincCandle *candle);
void create_scale(CandleData *data, gdouble price, gboolean prepend);
void setup_cartesian(GtkBox *parent, BincData *bincdata);

#endif // SYMBOLS_H