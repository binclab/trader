#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <gtk/gtk.h>
#include <sqlite3.h>

#include <string.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

typedef struct
{
    int allow_forward_starting;
    int delay_amount;
    char *display_name;
    int display_order;
    int exchange_is_open;
    char *exchange_name;
    int intraday_interval_minutes;
    int is_trading_suspended;
    char *market;
    char *market_display_name;
    double pip;
    char *quoted_currency_symbol;
    double spot;
    char *spot_age;
    char *spot_percentage_change;
    char *spot_time;
    char *subgroup;
    char *subgroup_display_name;
    char *submarket;
    char *submarket_display_name;
    char *symbol;
    char *symbol_type;
} Symbol;

#define SYMBOL_BOOM300 "BOOM300N"
#define SYMBOL_BOOM500 "BOOM500"
#define SYMBOL_BOOM1000 "BOOM1000"
#define SYMBOL_CRASH300 "CRASH300N"
#define SYMBOL_CRASH500 "CRASH500"
#define SYMBOL_CRASH1000 "CRASH1000"
#define SYMBOL_VI75 "R_75"
#define SYMBOL_GBPUSD "frxGBPUSD"
#define SYMBOL_GOLDBASKET "WLDXAU"

#endif // SYMBOLS_H