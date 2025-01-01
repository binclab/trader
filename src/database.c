#include "database.h"

int restore_candles(CandleInfo *info)
{
    int length = strlen(info->home) + strlen(info->instrument->symbol) + 15;
    char path[length];
    snprintf(path, length, "%sdatabases/%s.db", info->home, info->instrument->symbol);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(database));
        return info->count;
    }
    size_t maxlength = 50 + strlen(info->timeframe);
    char *sql_get_size = (char *)malloc(maxlength);
    snprintf(sql_get_size, maxlength, "SELECT epoch FROM \"%s\" ORDER BY epoch DESC LIMIT 1;", info->timeframe);
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(database, sql_get_size, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(database));
        return info->count;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        return info->count;
    }

    GDateTime *candletime = g_date_time_new_from_unix_utc(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);

    GTimeSpan span = g_date_time_difference(candletime, g_date_time_new_now_utc());
    g_date_time_unref(candletime);
    guint count = (int)(span / G_TIME_SPAN_MINUTE);
    if (count > info->count)
        return info->count;
    else
    {
        size_t maxlength = 53 + strlen(info->timeframe);
        char *sql_get_candles = (char *)malloc(maxlength);
        char *affix = "ORDER BY epoch DESC LIMIT";
        snprintf(sql_get_size, maxlength, "SELECT epoch FROM \"%s\" %s %i;", info->timeframe, affix, count);
        rc = sqlite3_prepare_v2(database, sql_get_candles, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(database));
            return info->count;
        }
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW)
        {
            return info->count;
        }

        info->candles = malloc(info->count * sizeof(Candle *));
        
        for (size_t index = count; index >= 0; index--)
        {

            info->candles[index] = malloc(sizeof(Candle));
            info->candles[index]->price = malloc(sizeof(CandlePrice));
            if (index == count)
            {
                info->candles[index]->time = malloc(sizeof(CandleTime));
                info->candles[index]->data = malloc(sizeof(CandleData));
                /*
                info->candles[index]->box = window->box;
                info->candles[index]->buffer = window->buffer;
                info->candles[index]->program = window->program;
                info->candles[index]->vertex = window->vertex;
                info->candles[index]->fragment = window->fragment;
                */
            }
            info->candles[index]->price->epoch = g_date_time_new_from_unix_utc(sqlite3_column_int64(stmt, 0));
            info->candles[index]->price->open = sqlite3_column_double(stmt, 1);
            info->candles[index]->price->close = sqlite3_column_double(stmt, 2);
            info->candles[index]->price->high = sqlite3_column_double(stmt, 3);
            info->candles[index]->price->low = sqlite3_column_double(stmt, 4);
        }

        return info->count - count;
    }
}

void *save_history(void *data)
{
    CandleInfo *info = (CandleInfo *)data;
    for (size_t i = 1; i < info->count; i++)
    {
        int length = strlen(info->home) + strlen(info->instrument->symbol) + 15;
        char path[length];
        snprintf(path, length, "%sdatabases/%s.db", info->home, info->instrument->symbol);
        sqlite3 *database;
        int rc = sqlite3_open(path, &database);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(database));
            return NULL;
        }
        size_t maxlength = 115 + strlen(info->timeframe);
        char *sql_symbol_history = (char *)malloc(maxlength);
        char *suffix = "epoch INTEGER PRIMARY KEY, open INTEGER, close INTEGER, high INTEGER, low INTEGER);";
        snprintf(sql_symbol_history, maxlength, "CREATE TABLE IF NOT EXISTS \"%s\" (%s", info->timeframe, suffix);
        char *err_msg = NULL;
        rc = sqlite3_exec(database, sql_symbol_history, 0, 0, &err_msg);
        free(sql_symbol_history);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            sqlite3_close(database);
            return NULL;
        }
        suffix = "epoch, open, close, high, low) VALUES (?, ?, ?, ?, ?);";
        maxlength = 81 + strlen(info->timeframe);
        char *sql_insert = (char *)malloc(maxlength);
        snprintf(sql_insert, maxlength, "INSERT OR IGNORE INTO \"%s\" (%s", info->timeframe, suffix);

        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(database));
            sqlite3_close(database);
            return NULL;
        }

        sqlite3_bind_int64(stmt, 1, g_date_time_to_unix(info->candles[i]->price->epoch));
        sqlite3_bind_double(stmt, 2, info->candles[i]->price->open);
        sqlite3_bind_double(stmt, 3, info->candles[i]->price->close);
        sqlite3_bind_double(stmt, 4, info->candles[i]->price->high);
        sqlite3_bind_double(stmt, 5, info->candles[i]->price->low);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(database));
            sqlite3_finalize(stmt);
            sqlite3_close(database);
            return NULL;
        }

        sqlite3_finalize(stmt);
        sqlite3_close(database);
    }
    free(info);

    return NULL;
}

void create_symbol_database(char *home)
{
    int length = strlen(home) + 22;
    char *path = (char *)malloc(length);
    snprintf(path, length - 1, "%sdatabases/symbols.db", home);
    GFile *file = g_file_new_for_path(path);
    GFile *parent = g_file_get_parent(file);
    if (!g_file_query_exists(parent, NULL))
        g_file_make_directory_with_parents(parent, NULL, NULL);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    free(path);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(EXIT_FAILURE);
    }

    // SQL statement to create the Symbols table
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

    // SQL statement to create the Session table
    const char *sql_create_session = "CREATE TABLE IF NOT EXISTS Session ("
                                     "symbol TEXT PRIMARY KEY"
                                     ");";

    // Execute the SQL statement
    char *err_msg = NULL;
    rc = sqlite3_exec(database, sql_create_symbols, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(database);
        exit(EXIT_FAILURE);
    }
    rc = sqlite3_exec(database, sql_create_session, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(database);
        exit(EXIT_FAILURE);
    }
    sqlite3_close(database);
}

Symbol *restore_last_instrument(char *home)
{
    int length = strlen(home) + 21;
    char path[length];
    snprintf(path, length, "%sdatabases/symbols.db", home);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(database));
        return NULL;
    }
    char *sql = "SELECT symbol FROM Session LIMIT 1;";

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to restore Instrument: %s\n", sqlite3_errmsg(database));
        return NULL;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        return NULL;
    }

    char *buffer = strdup((const char *)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    sql = "SELECT * FROM Symbols WHERE symbol = ?;";
    rc = sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        free(buffer);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, buffer, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    free(buffer);
    if (rc != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }

    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        sqlite3_finalize(stmt);
        return NULL;
    }

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

void update_active_symbols(char *home, JsonArray *list)
{
    int length = strlen(home) + 21;
    char path[length];
    snprintf(path, length, "%sdatabases/symbols.db", home);
    sqlite3 *database;
    int rc = sqlite3_open(path, &database);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(database));
        sqlite3_close(database);
    }
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

    for (guint i = 0; i < json_array_get_length(list); i++)
    {
        JsonObject *object = json_array_get_object_element(list, i);
        int rc = sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(database));
            continue;
        }

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
        if (rc != SQLITE_DONE)
        {
            fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(database));
        }

        sqlite3_finalize(stmt);
    }

    sqlite3_exec(database, "COMMIT;", NULL, NULL, NULL);
    sqlite3_close(database);
}

void set_default_instrument(char *home)
{
    int length = strlen(home) + 21;
    char path[length];
    snprintf(path, length, "%sdatabases/symbols.db", home);
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

static int update_symbol(sqlite3 *db, Symbol *symbol)
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