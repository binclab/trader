#include "database.h"

int restore_candles(BincData *bincdata)
{
    int length = strlen(bincdata->home) + strlen(bincdata->instrument->symbol) + 13;
    char path[length];
    snprintf(path, length, "%shistory/%s.db", bincdata->home, bincdata->instrument->symbol);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    size_t maxlength = 50 + strlen(bincdata->timeframe);
    char *sql_get_size = (char *)malloc(maxlength);
    snprintf(sql_get_size, maxlength, "SELECT epoch FROM \"%s\" ORDER BY epoch DESC LIMIT 1;", bincdata->timeframe);
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(database, sql_get_size, -1, &stmt, NULL);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        return bincdata->count;
    }

    GDateTime *candletime = g_date_time_new_from_unix_utc(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);

    GTimeSpan span = g_date_time_difference(candletime, g_date_time_new_now_utc());
    g_date_time_unref(candletime);
    guint count = (int)(span / G_TIME_SPAN_MINUTE);
    if (count > bincdata->count)
        return bincdata->count;
    else
    {
        size_t maxlength = 53 + strlen(bincdata->timeframe);
        char *sql_get_candles = (char *)malloc(maxlength);
        char *affix = "ORDER BY epoch DESC LIMIT";
        snprintf(sql_get_size, maxlength, "SELECT epoch FROM \"%s\" %s %i;", bincdata->timeframe, affix, count);
        rc = sqlite3_prepare_v2(database, sql_get_candles, -1, &stmt, NULL);
        rc = sqlite3_step(stmt);

        for (size_t index = 0; index > count; index++)
        {
            BincCandle *candle = g_object_new(BINC_TYPE_CANDLE, NULL);
            candle->data = bincdata->data;
            candle->time = bincdata->time;
            candle->price->epoch = g_date_time_new_from_unix_utc(sqlite3_column_int64(stmt, 0));
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
    GListModel *model = (GListModel *)bincdata->store;
    int items = g_list_model_get_n_items(model);

    int length = strlen(bincdata->home) + strlen(bincdata->instrument->symbol) + 13;
    char *path = (char *)malloc(length);
    snprintf(path, length, "%shistory/%s.db", bincdata->home, bincdata->instrument->symbol);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    free(path);
    size_t maxlength = 115 + strlen(bincdata->timeframe);
    char *sql_symbol_history = (char *)malloc(maxlength);
    char *suffix = "epoch INTEGER PRIMARY KEY, open INTEGER, close INTEGER, high INTEGER, low INTEGER);";
    snprintf(sql_symbol_history, maxlength, "CREATE TABLE IF NOT EXISTS \"%s\" (%s", bincdata->timeframe, suffix);
    char *err_msg = NULL;
    rc = sqlite3_exec(database, sql_symbol_history, 0, 0, &err_msg);
    free(sql_symbol_history);
    suffix = "epoch, open, close, high, low) VALUES (?, ?, ?, ?, ?);";
    maxlength = 81 + strlen(bincdata->timeframe);
    char *sql_insert = (char *)malloc(maxlength);
    snprintf(sql_insert, maxlength, "INSERT OR IGNORE INTO \"%s\" (%s", bincdata->timeframe, suffix);
    for (size_t position = items - bincdata->count; position < items; position++)
    {
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);
        BincCandle *candle = g_list_model_get_item((GListModel *)model, position);

        sqlite3_bind_int64(stmt, 1, g_date_time_to_unix(candle->price->epoch));
        sqlite3_bind_double(stmt, 2, candle->price->open);
        sqlite3_bind_double(stmt, 3, candle->price->close);
        sqlite3_bind_double(stmt, 4, candle->price->high);
        sqlite3_bind_double(stmt, 5, candle->price->low);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(database);
}

static void create_symbol_database(sqlite3 *database)
{
    const char *sql_create_symbols = "CREATE TABLE IF NOT EXISTS Symbols ("
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

    const char *sql_create_session = "CREATE TABLE IF NOT EXISTS Session ("
                                     "symbol TEXT PRIMARY KEY"
                                     ");";

    char *err_msg = NULL;
    sqlite3_exec(database, sql_create_symbols, 0, 0, &err_msg);
    sqlite3_exec(database, sql_create_session, 0, 0, &err_msg);
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR IGNORE INTO Session (symbol) VALUES (?);";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, SYMBOL_BOOM1000, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(database);
}

BincSymbol *restore_last_instrument(BincData *bincdata)
{
    int length = strlen(bincdata->home) + 11;
    char path[length];
    snprintf(path, length, "%ssession.db", bincdata->home);
    sqlite3_stmt *stmt;
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);

    char *sql = "SELECT symbol FROM Session LIMIT 1;";
    rc = sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    rc = sqlite3_step(stmt);

    char *buffer = strdup((const char *)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    sql = "SELECT * FROM Symbols WHERE symbol = ?;";
    rc = sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, buffer, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    free(buffer);

    BincSymbol *symbol = malloc(sizeof(BincSymbol));

    symbol->symbol = strdup((const char *)sqlite3_column_text(stmt, 0));
    symbol->display_name = strdup((const char *)sqlite3_column_text(stmt, 1));
    symbol->allow_forward_starting = sqlite3_column_int(stmt, 2);
    symbol->delay_amount = sqlite3_column_int(stmt, 3);
    symbol->display_order = sqlite3_column_int(stmt, 4);
    symbol->exchange_is_open = sqlite3_column_int(stmt, 5);
    symbol->exchange_name = strdup((const char *)sqlite3_column_text(stmt, 6));
    symbol->intraday_interval_minutes = sqlite3_column_int(stmt, 7);
    symbol->is_trading_suspended = sqlite3_column_int(stmt, 8);
    symbol->market = strdup((const char *)sqlite3_column_text(stmt, 9));
    symbol->market_display_name = strdup((const char *)sqlite3_column_text(stmt, 10));
    symbol->pip = sqlite3_column_double(stmt, 11);
    symbol->quoted_currency_symbol = strdup((const char *)sqlite3_column_text(stmt, 12));
    symbol->spot = sqlite3_column_double(stmt, 13);
    symbol->spot_age = strdup((const char *)sqlite3_column_text(stmt, 14));
    symbol->spot_percentage_change = strdup((const char *)sqlite3_column_text(stmt, 15));
    symbol->spot_time = strdup((const char *)sqlite3_column_text(stmt, 16));
    symbol->subgroup = strdup((const char *)sqlite3_column_text(stmt, 17));
    symbol->subgroup_display_name = strdup((const char *)sqlite3_column_text(stmt, 18));
    symbol->submarket = strdup((const char *)sqlite3_column_text(stmt, 19));
    symbol->submarket_display_name = strdup((const char *)sqlite3_column_text(stmt, 20));
    symbol->symbol_type = strdup((const char *)sqlite3_column_text(stmt, 21));
    sqlite3_finalize(stmt);
    sqlite3_close(database);
    return symbol;
}

static void create_default_tables(sqlite3 *database)
{
    const char *sql_create_accounts = "CREATE TABLE IF NOT EXISTS Accounts ("
                                      "account TEXT PRIMARY KEY,"
                                      "currency TEXT"
                                      ");";

    const char *sql_create_symbols = "CREATE TABLE IF NOT EXISTS Symbols ("
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

    const char *sql_create_session = "CREATE TABLE IF NOT EXISTS Session ("
                                     "symbol TEXT PRIMARY KEY,"
                                     "profile INTEGER"
                                     ");";

    char *err_msg = NULL;
    sqlite3_exec(database, sql_create_accounts, 0, 0, &err_msg);
    sqlite3_exec(database, sql_create_symbols, 0, 0, &err_msg);
    sqlite3_exec(database, sql_create_session, 0, 0, &err_msg);
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR IGNORE INTO Session (symbol, profile) VALUES ('BOOM1000', 0);";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(database);
}

void save_token_attributes(GTask *task, gpointer source, gpointer data, GCancellable *unused)
{
    BincData *bincdata = (BincData *)data;

    int length = strlen(bincdata->home) + 11;
    char path[length];
    snprintf(path, length, "%ssession.db", bincdata->home);
    sqlite3_stmt *stmt;
    sqlite3 *database;
    sqlite3_open(path, &database);
    const char *sql = "INSERT OR IGNORE INTO Accounts (account, currency) VALUES (?, ?);";

    for (int index = 0; index < 3; index++)
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
    int length = strlen(bincdata->home) + 11;
    char *path = (char *)malloc(length);
    snprintf(path, length, "%ssession.db", bincdata->home);
    GFile *file = g_file_new_for_path(path);
    sqlite3_stmt *stmt;
    sqlite3 *database;
    if (!g_file_query_exists(file, NULL))
    {
        sqlite3_open(path, &database);
        create_default_tables(database);
        snprintf(path, length, "%shistory", bincdata->home);
        GFile *history = g_file_new_for_path(path);
        if (!g_file_query_exists(history, NULL))
            g_file_make_directory(history, NULL, NULL);
        free(path);
        g_task_run_in_thread(bincdata->task, setup_soup_session);
        gtk_window_set_child(bincdata->window, bincdata->webview);
        return FALSE;
    }
    sqlite3_open(path, &database);
    free(path);
    char *sql = "SELECT account, currency FROM Accounts LIMIT 3;";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    for (int index = 0; index < 3; index++)
    {
        sqlite3_step(stmt);
        bincdata->account[index]->account = g_strdup((const char *)sqlite3_column_text(stmt, 0));
        bincdata->account[index]->currency = g_strdup((const char *)sqlite3_column_text(stmt, 1));
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(database);
    return TRUE;
}

void update_active_symbols(BincData *bincdata, JsonArray *list)
{
    int length = strlen(bincdata->home) + 11;
    char path[length];
    snprintf(path, length, "%ssession.db", bincdata->home);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    sqlite3_stmt *stmt;
    const char *sql_insert = "INSERT OR IGNORE INTO Symbols ("
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
        rc = sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);

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
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(database, "COMMIT;", NULL, NULL, NULL);
    sqlite3_close(database);
}

void set_default_instrument(char *home)
{
    int length = strlen(home) + 11;
    char path[length];
    snprintf(path, length, "%ssession.db", home);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(database));
        exit(EXIT_FAILURE);
    }
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR IGNORE INTO Session (symbol) VALUES (?);";
    rc = sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(database));
        return;
    }

    sqlite3_bind_text(stmt, 1, SYMBOL_BOOM1000, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(database));
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);
}

static int update_symbol(sqlite3 *db, BincSymbol *symbol)
{
    const char *sql_update = "UPDATE Symbols SET "
                             "allow_forward_starting = ?, "
                             "delay_amount = ?, "
                             "display_name = ?, "
                             "display_order = ?, "
                             "exchange_is_open = ?, "
                             "exchange_name = ?, "
                             "intraday_interval_minutes = ?, "
                             "is_trading_suspended = ?, "
                             "market = ?, "
                             "market_display_name = ?, "
                             "pip = ?, "
                             "quoted_currency_symbol = ?, "
                             "spot = ?, "
                             "spot_age = ?, "
                             "spot_percentage_change = ?, "
                             "spot_time = ?, "
                             "subgroup = ?, "
                             "subgroup_display_name = ?, "
                             "submarket = ?, "
                             "submarket_display_name = ?, "
                             "symbol_type = ? "
                             "WHERE symbol = ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // Bind values to the statement
    sqlite3_bind_int(stmt, 1, symbol->allow_forward_starting);
    sqlite3_bind_int(stmt, 2, symbol->delay_amount);
    sqlite3_bind_text(stmt, 3, symbol->display_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, symbol->display_order);
    sqlite3_bind_int(stmt, 5, symbol->exchange_is_open);
    sqlite3_bind_text(stmt, 6, symbol->exchange_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, symbol->intraday_interval_minutes);
    sqlite3_bind_int(stmt, 8, symbol->is_trading_suspended);
    sqlite3_bind_text(stmt, 9, symbol->market, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, symbol->market_display_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 11, symbol->pip);
    sqlite3_bind_text(stmt, 12, symbol->quoted_currency_symbol, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 13, symbol->spot);
    sqlite3_bind_text(stmt, 14, symbol->spot_age, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 15, symbol->spot_percentage_change, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 16, symbol->spot_time, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 17, symbol->subgroup, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 18, symbol->subgroup_display_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 19, symbol->submarket, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 20, symbol->submarket_display_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 21, symbol->symbol_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 22, symbol->symbol, -1, SQLITE_TRANSIENT);

    // Execute the statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    // Finalize the statement
    sqlite3_finalize(stmt);

    return rc;
}