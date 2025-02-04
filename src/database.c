#include "database.h"
#include "cleanup.h"

gint restore_candles(BincData *bincdata)
{
    gchar *timeframe = bincdata->timeframe;
    gchar *path = g_strdup_printf("%shistory/%s.db", bincdata->home, bincdata->instrument->symbol);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_free(path);
    gchar *suffix = "ORDER BY epoch DESC LIMIT 1;";
    gchar *instruction = g_strdup_printf("SELECT epoch FROM \"%s\" %s", timeframe, suffix);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(database, instruction, -1, &stmt, NULL);
    g_free(instruction);
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        return bincdata->count;
    }

    GDateTime *candletime = g_date_time_new_from_unix_utc(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);

    GTimeSpan span = g_date_time_difference(candletime, g_date_time_new_now_utc());
    g_date_time_unref(candletime);
    guint count = (gint)(span / G_TIME_SPAN_MINUTE);
    if (count > bincdata->count)
        return bincdata->count;
    else
    {
        gchar *prefix = "SELECT epoch FROM";
        gchar *affix = "ORDER BY epoch DESC LIMIT";
        gchar *sql_get_candles = g_strdup_printf("%s \"%s\" %s %i;", prefix, timeframe, affix, count);
        sqlite3_prepare_v2(database, sql_get_candles, -1, &stmt, NULL);
        g_free(sql_get_candles);
        sqlite3_step(stmt);
        for (size_t index = 0; index > count; index++)
        {
            BincCandle *candle = g_object_new(BINC_TYPE_CANDLE, NULL);
            candle->data = bincdata->data;
            candle->time = bincdata->time;
            candle->price->epoch = sqlite3_column_int64(stmt, 0);
            candle->price->open = sqlite3_column_double(stmt, 1);
            candle->price->close = sqlite3_column_double(stmt, 2);
            candle->price->high = sqlite3_column_double(stmt, 3);
            candle->price->low = sqlite3_column_double(stmt, 4);
            g_list_store_append(bincdata->store, candle);
        }

        return bincdata->count - count;
    }
}

void save_history(GTask *task, gpointer source, gpointer data, GCancellable *unused)
{
    BincData *bincdata = (BincData *)data;
    GListModel *model = G_LIST_MODEL(bincdata->store);

    gchar *path = g_strdup_printf("%shistory/%s.db", bincdata->home, bincdata->instrument->symbol);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_free(path);
    gchar *suffix = "epoch INTEGER PRIMARY KEY, open INTEGER, close INTEGER, high INTEGER, low INTEGER);";
    gchar *timeframe = bincdata->timeframe;
    gchar *sql_symbol_history = g_strdup_printf("CREATE TABLE IF NOT EXISTS \"%s\" (%s", timeframe, suffix);
    gchar *err_msg = NULL;
    sqlite3_exec(database, sql_symbol_history, 0, 0, &err_msg);
    g_free(sql_symbol_history);
    suffix = "epoch, open, close, high, low) VALUES (?, ?, ?, ?, ?);";
    gchar *sql_insert = g_strdup_printf("INSERT OR IGNORE INTO \"%s\" (%s", timeframe, suffix);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);
    g_free(sql_insert);
    gint items = g_list_model_get_n_items(model);
    for (gint position = items - bincdata->count; position < items; position++)
    {
        BincCandle *candle = g_list_model_get_item(model, position);

        sqlite3_bind_int64(stmt, 1, candle->price->epoch);
        sqlite3_bind_double(stmt, 2, candle->price->open);
        sqlite3_bind_double(stmt, 3, candle->price->close);
        sqlite3_bind_double(stmt, 4, candle->price->high);
        sqlite3_bind_double(stmt, 5, candle->price->low);

        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(database);
}

void restore_last_instrument(BincData *bincdata)
{
    gchar *path = g_strdup_printf("%ssession.db", bincdata->home);
    sqlite3_stmt *stmt;
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_free(path);

    gchar *sql = "SELECT symbol FROM Session LIMIT 1;";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);

    gchar *buffer = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    sql = "SELECT * FROM Symbols WHERE symbol = ?;";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, buffer, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    g_free(buffer);

    if (bincdata->instrument)
    {
        free_instrument(bincdata->instrument);
        bincdata->instrument = NULL;
    }

    BincSymbol *symbol = g_new0(BincSymbol, 1);

    symbol->symbol = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
    symbol->display_name = g_strdup((const gchar *)sqlite3_column_text(stmt, 1));
    symbol->allow_forward_starting = sqlite3_column_int(stmt, 2);
    symbol->delay_amount = sqlite3_column_int(stmt, 3);
    symbol->display_order = sqlite3_column_int(stmt, 4);
    symbol->exchange_is_open = sqlite3_column_int(stmt, 5);
    symbol->exchange_name = g_strdup((const gchar *)sqlite3_column_text(stmt, 6));
    symbol->intraday_interval_minutes = sqlite3_column_int(stmt, 7);
    symbol->is_trading_suspended = sqlite3_column_int(stmt, 8);
    symbol->market = g_strdup((const gchar *)sqlite3_column_text(stmt, 9));
    symbol->market_display_name = g_strdup((const gchar *)sqlite3_column_text(stmt, 10));

    gchar *degree = g_strdup_printf("%f\n", sqlite3_column_double(stmt, 11));
    symbol->pip = g_utf8_strchr(degree, -1, '1') - g_utf8_strchr(degree, -1, '.');
    g_free(degree);
    symbol->quoted_currency_symbol = g_strdup((const gchar *)sqlite3_column_text(stmt, 12));
    symbol->spot = sqlite3_column_double(stmt, 13);
    symbol->spot_age = g_strdup((const gchar *)sqlite3_column_text(stmt, 14));
    symbol->spot_percentage_change = g_strdup((const gchar *)sqlite3_column_text(stmt, 15));
    symbol->spot_time = g_strdup((const gchar *)sqlite3_column_text(stmt, 16));
    symbol->subgroup = g_strdup((const gchar *)sqlite3_column_text(stmt, 17));
    symbol->subgroup_display_name = g_strdup((const gchar *)sqlite3_column_text(stmt, 18));
    symbol->submarket = g_strdup((const gchar *)sqlite3_column_text(stmt, 19));
    symbol->submarket_display_name = g_strdup((const gchar *)sqlite3_column_text(stmt, 20));
    symbol->symbol_type = g_strdup((const gchar *)sqlite3_column_text(stmt, 21));
    sqlite3_finalize(stmt);
    sqlite3_close(database);

    bincdata->instrument = symbol;

    if (bincdata->time == NULL)
    {
        bincdata->time = g_new0(CandleTime, 1);
        GDateTime *timelocal = g_date_time_new_now_local();
        GDateTime *timeutc = g_date_time_new_now_utc();

        bincdata->time->hours = g_date_time_get_hour(timelocal) - g_date_time_get_hour(timeutc);
        bincdata->time->minutes = g_date_time_get_minute(timelocal) - g_date_time_get_minute(timeutc);
        g_date_time_unref(timelocal);
        g_date_time_unref(timeutc);
    }
}

static void create_default_tables(sqlite3 *database)
{
    const gchar *sql_create_accounts = "CREATE TABLE IF NOT EXISTS Accounts ("
                                       "account TEXT PRIMARY KEY,"
                                       "currency TEXT"
                                       ");";

    const gchar *sql_create_symbols = "CREATE TABLE IF NOT EXISTS Symbols ("
                                      "symbol TEXT PRIMARY KEY,"
                                      "display_name TEXT,"
                                      "allow_forward_starting BOOLEAN,"
                                      "delay_amount INTEGER,"
                                      "display_order INTEGER,"
                                      "exchange_is_open BOOLEAN,"
                                      "exchange_name TEXT,"
                                      "intraday_interval_minutes INTEGER,"
                                      "is_trading_suspended BOOLEAN,"
                                      "market TEXT,"
                                      "market_display_name TEXT,"
                                      "pip REAL,"
                                      "quoted_currency_symbol TEXT,"
                                      "spot REAL,"
                                      "spot_age TEXT,"
                                      "spot_percentage_change TEXT,"
                                      "spot_time TEXT,"
                                      "subgroup TEXT,"
                                      "subgroup_display_name TEXT,"
                                      "submarket TEXT,"
                                      "submarket_display_name TEXT,"
                                      "symbol_type TEXT"
                                      ");";

    const gchar *sql_create_session = "CREATE TABLE IF NOT EXISTS Session ("
                                      "symbol TEXT PRIMARY KEY,"
                                      "profile INTEGER"
                                      ");";

    const gchar *sql_create_favourites = "CREATE TABLE IF NOT EXISTS Favourites ("
                                         "symbol TEXT PRIMARY KEY,"
                                         "name TEXT"
                                         ");";

    gchar *err_msg = NULL;
    sqlite3_exec(database, sql_create_accounts, 0, 0, &err_msg);
    sqlite3_exec(database, sql_create_symbols, 0, 0, &err_msg);
    sqlite3_exec(database, sql_create_session, 0, 0, &err_msg);
    sqlite3_exec(database, sql_create_favourites, 0, 0, &err_msg);
    sqlite3_stmt *stmt;
    gchar *sql = "INSERT OR IGNORE INTO Session (symbol, profile) VALUES ('BOOM1000', 0);";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    sql = "INSERT OR IGNORE INTO Favourites (symbol, name) VALUES (?, ?);";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    gchar *list[4][2] = {{SYMBOL_BOOM1000, "Boom 1000 Index"},
                         {SYMBOL_CRASH1000, "Crash 1000 Index"},
                         {SYMBOL_VI75, "Volatility 75 Index"},
                         {SYMBOL_EURUSD, "EUR/USD"}};
    for (gint index = 0; index < 4; index++)
    {
        sqlite3_bind_text(stmt, 1, list[index][0], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, list[index][1], -1, SQLITE_STATIC);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(database);
}

void save_token_attributes(GTask *task, gpointer source, gpointer data, GCancellable *unused)
{
    BincData *bincdata = (BincData *)data;

    gint length = strlen(bincdata->home) + 11;
    gchar path[length];
    snprintf(path, length, "%ssession.db", bincdata->home);
    sqlite3_stmt *stmt;
    sqlite3 *database;
    sqlite3_open(path, &database);
    const gchar *sql = "INSERT OR IGNORE INTO Accounts (account, currency) VALUES (?, ?);";

    for (gint index = 0; index < 3; index++)
    {
        sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, bincdata->account[index]->account, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, bincdata->account[index]->currency, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(database);
};

gboolean get_token_attributes(BincData *bincdata)
{
    gchar *path = g_strdup_printf("%ssession.db", bincdata->home);
    GFile *file = g_file_new_for_path(path);
    sqlite3_stmt *stmt;
    sqlite3 *database;
    gboolean firstrun = !g_file_query_exists(file, NULL);
    sqlite3_open(path, &database);
    g_free(path);
    if (firstrun)
    {
        create_default_tables(database);
        path = g_strdup_printf("%shistory", bincdata->home);
        GFile *history = g_file_new_for_path(path);
        if (!g_file_query_exists(history, NULL))
            g_file_make_directory(history, NULL, NULL);
        g_free(path);
        g_task_run_in_thread(bincdata->task, setup_soup_session);
        gtk_window_set_child(bincdata->window, bincdata->webview);
        return FALSE;
    }
    gchar *sql = "SELECT account, currency FROM Accounts LIMIT 3;";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    for (gint index = 0; index < 3; index++)
    {
        sqlite3_step(stmt);
        bincdata->account[index]->account = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
        bincdata->account[index]->currency = g_strdup((const gchar *)sqlite3_column_text(stmt, 1));
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(database);
    return TRUE;
}

void update_active_symbols(BincData *bincdata, JsonArray *list)
{
    gint length = strlen(bincdata->home) + 11;
    gchar path[length];
    snprintf(path, length, "%ssession.db", bincdata->home);
    sqlite3 *database;
    sqlite3_open(path, &database);
    sqlite3_stmt *stmt;
    const gchar *sql_insert = "INSERT OR IGNORE INTO Symbols ("
                              "symbol, display_name, allow_forward_starting, delay_amount, "
                              "display_order, exchange_is_open, exchange_name, intraday_interval_minutes, "
                              "is_trading_suspended, market, market_display_name, pip, "
                              "quoted_currency_symbol, spot, spot_age, spot_percentage_change, "
                              "spot_time, subgroup, subgroup_display_name, submarket, "
                              "submarket_display_name, symbol_type) "
                              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_exec(database, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    for (guint index = 0; index < json_array_get_length(list); index++)
    {
        JsonObject *object = json_array_get_object_element(list, index);
        sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);

        sqlite3_bind_text(stmt, 1, json_object_get_string_member(object, "symbol"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, json_object_get_string_member(object, "display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, json_object_get_int_member(object, "allow_forward_starting"));
        sqlite3_bind_int(stmt, 4, json_object_get_int_member(object, "delay_amount"));
        sqlite3_bind_int(stmt, 5, json_object_get_int_member(object, "display_order"));
        sqlite3_bind_int(stmt, 6, json_object_get_int_member(object, "exchange_is_open"));
        sqlite3_bind_text(stmt, 7, json_object_get_string_member(object, "exchange_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 8, json_object_get_int_member(object, "intraday_interval_minutes"));
        sqlite3_bind_int(stmt, 9, json_object_get_int_member(object, "is_trading_suspended"));
        sqlite3_bind_text(stmt, 10, json_object_get_string_member(object, "market"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 11, json_object_get_string_member(object, "market_display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 12, json_object_get_double_member(object, "pip"));
        sqlite3_bind_text(stmt, 13, json_object_get_string_member(object, "quoted_currency_symbol"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 14, json_object_get_double_member(object, "spot"));
        sqlite3_bind_text(stmt, 15, json_object_get_string_member(object, "spot_age"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 16, json_object_get_string_member(object, "spot_percentage_change"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 17, json_object_get_string_member(object, "spot_time"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 18, json_object_get_string_member(object, "subgroup"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 19, json_object_get_string_member(object, "subgroup_display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 20, json_object_get_string_member(object, "submarket"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 21, json_object_get_string_member(object, "submarket_display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 22, json_object_get_string_member(object, "symbol_type"), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(database, "COMMIT;", NULL, NULL, NULL);
    sqlite3_close(database);
}

void insert_favourites(GTask *task, gpointer source, gpointer user_data, GCancellable *unused)
{
    BincData *bincdata = BINC_DATA(user_data);
    gchar *path = g_strdup_printf("%ssession.db", bincdata->home);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_free(path);
    sqlite3_stmt *stmt;
    const gchar *sql = "INSERT OR IGNORE INTO Favourites (symbol, name) VALUES (?, ?);";

    GListModel *model = gtk_drop_down_get_model(bincdata->symbolbox);

    for (guint index = 0; index < g_list_model_get_n_items(model); index++)
    {
        GtkStringObject *object = GTK_STRING_OBJECT(g_list_model_get_item(model, index));
        const gchar *name = gtk_string_object_get_string(object);
        gchar *symbol = (gchar *)g_object_get_data(G_OBJECT(object), "symbol");
        sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, symbol, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(database);
}

void set_default_instrument(gchar *home, const gchar *symbol, const gchar *instrument)
{
    gchar *path = g_strdup_printf("%ssession.db", home);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_free(path);
    gchar *prefix = "UPDATE Session SET symbol =";
    const gchar *sql = g_strdup_printf("%s '%s' WHERE symbol = '%s';", prefix, symbol, instrument);
    sqlite3_exec(database, sql, 0, 0, NULL);
    sqlite3_close(database);
}

void populate_favourite_symbols(GTask *task, gpointer source, gpointer user_data, GCancellable *unused)
{
    BincData *bincdata = BINC_DATA(user_data);
    gchar *path = g_strdup_printf("%ssession.db", bincdata->home);
    sqlite3_stmt *stmt;
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_free(path);

    gchar *sql = "SELECT * FROM Favourites;";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);

    GListStore *model = G_LIST_STORE(gtk_drop_down_get_model(bincdata->symbolbox));

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        gchar *symbol = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
        const gchar *name = g_strdup((const gchar *)sqlite3_column_text(stmt, 1));
        GtkStringObject *object = gtk_string_object_new(name);
        g_object_set_data(G_OBJECT(object), "symbol", symbol);
        g_list_store_append(model, object);
    }
}