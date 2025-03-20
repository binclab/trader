#include "database.h"
#include "session.h"
#include "cleanup.h"

gint get_saved_candles(GObject *object, const gchar *symbol, const gchar *timeframe)
{
    const gchar *home = gtk_string_object_get_string(GTK_STRING_OBJECT(object));
    gsize maxlength = strlen(home) + strlen(symbol) + 12;
    gchar *path = (gchar *)g_malloc0(maxlength);
    g_snprintf(path, maxlength, "%shistory/%s.db", home, symbol);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_clear_pointer(&path, g_free);

    maxlength = 24 + strlen(timeframe);
    gchar *instruction = (gchar *)g_malloc0(maxlength);
    g_snprintf(instruction, maxlength, "SELECT COUNT(*) FROM \"%s\";", timeframe);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(database, instruction, -1, &stmt, NULL);
    g_clear_pointer(&instruction, g_free);
    gint fetch = 1440;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        gint rows = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        gchar *suffix = "ORDER BY epoch DESC LIMIT 1;";
        maxlength = strlen(timeframe) + 53;
        instruction = (gchar *)g_malloc0(maxlength);
        g_snprintf(instruction, maxlength, "SELECT epoch FROM \"%s\" %s", timeframe, suffix);
        sqlite3_prepare_v2(database, instruction, -1, &stmt, NULL);
        g_clear_pointer(&instruction, g_free);
        sqlite3_step(stmt);
        if (g_str_equal(timeframe, "M1"))
        {
            gint64 epoch = sqlite3_column_int64(stmt, 0);
            GDateTime *candletime = g_date_time_new_from_unix_utc(epoch);
            GDateTime *timenow = g_date_time_new_now_utc();
            GTimeSpan span = g_date_time_difference(timenow, candletime);

            g_date_time_unref(candletime);
            g_date_time_unref(timenow);
            fetch = (gint)(span / G_TIME_SPAN_MINUTE);
        }

        sqlite3_finalize(stmt);

        if (fetch <= 1440)
        {
            gchar *prefix = "SELECT * FROM";
            gchar *affix = "ORDER BY epoch ASC LIMIT -1 OFFSET";
            maxlength = 64;
            instruction = (gchar *)g_malloc0(maxlength);
            gint count = rows - 1441 + fetch;
            g_snprintf(instruction, maxlength, "%s \"%s\" %s %i;", prefix, timeframe, affix, count);
            sqlite3_prepare_v2(database, instruction, -1, &stmt, NULL);
            g_clear_pointer(&instruction, g_free);
            GListStore *store = g_object_get_data(object, "candles");
            g_print("Loading history from Database\n");
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                GObject *candle = g_object_new(G_TYPE_OBJECT, NULL);
                GDateTime *epoch = g_date_time_new_from_unix_utc(sqlite3_column_int64(stmt, 0));
                gdouble *price = g_new(gdouble, 4);
                price[0] = sqlite3_column_double(stmt, 1);
                price[1] = sqlite3_column_double(stmt, 2);
                price[2] = sqlite3_column_double(stmt, 3);
                price[3] = sqlite3_column_double(stmt, 4);
                g_object_set_data(candle, "price", price);
                g_object_set_data(candle, "epoch", epoch);
                g_object_set_data(candle, "stat", g_object_get_data(object, "stat"));
                g_object_set_data(candle, "data", g_object_get_data(object, "data"));
                g_object_set_data(candle, "time", g_object_get_data(object, "time"));
                g_list_store_append(store, candle);
                g_date_time_new_now_local();
            }
            sqlite3_finalize(stmt);
            g_print("History loaded from Database\n");
        }
        else
        {
            fetch = 1440;
        }
    }
    else
        sqlite3_finalize(stmt);

    return fetch;
}

static gpointer set_timeframe(gint value)
{
    GtkStringObject *object = NULL;
    switch (value)
    {
    case 1:
        object = gtk_string_object_new("M5");
        break;
    case 2:
        object = gtk_string_object_new("M15");
        break;
    case 3:
        object = gtk_string_object_new("M30");
        break;
    case 4:
        object = gtk_string_object_new("H1");
        break;
    case 5:
        object = gtk_string_object_new("H2");
        break;
    case 6:
        object = gtk_string_object_new("D1");
        break;
    case 7:
        object = gtk_string_object_new("W1");
        break;
    case 8:
        object = gtk_string_object_new("MN1");
        break;
    default:
        object = gtk_string_object_new("M1");
        break;
    }

    return object;
}

void setup_symbol(GObject *task, sqlite3 *database)
{
    gchar *sql = "SELECT * FROM Session LIMIT 1;";
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);

    gchar *buffer = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
    GObject *profile = G_OBJECT(g_object_get_data(task, "profile"));
    g_object_set_data(profile, "id", GINT_TO_POINTER(sqlite3_column_int(stmt, 1)));
    g_object_set_data(task, "timeframe", set_timeframe(sqlite3_column_int(stmt, 2)));

    sqlite3_finalize(stmt);

    sql = "SELECT * FROM Symbols WHERE symbol = ?;";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, buffer, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);

    g_clear_pointer(&buffer, g_free);

    GtkStringObject *symbol = GTK_STRING_OBJECT(g_object_get_data(task, "symbol"));

    if (symbol)
    {
        free_instrument(G_OBJECT(symbol));
        symbol = NULL;
    }

    symbol = gtk_string_object_new(g_strdup((const gchar *)sqlite3_column_text(stmt, 0)));
    GObject *object = G_OBJECT(symbol);
    g_object_set_data(object, "display_name", g_strdup((const gchar *)sqlite3_column_text(stmt, 1)));
    g_object_set_data(object, "allow_forward_starting", GINT_TO_POINTER(sqlite3_column_int(stmt, 2)));
    g_object_set_data(object, "delay_amount", GINT_TO_POINTER(sqlite3_column_int(stmt, 3)));
    g_object_set_data(object, "display_order", GINT_TO_POINTER(sqlite3_column_int(stmt, 4)));
    g_object_set_data(object, "exchange_is_open", GINT_TO_POINTER(sqlite3_column_int(stmt, 5)));
    g_object_set_data(object, "exchange_name", g_strdup((const gchar *)sqlite3_column_text(stmt, 6)));
    g_object_set_data(object, "intraday_interval_minutes", GINT_TO_POINTER(sqlite3_column_int(stmt, 7)));
    g_object_set_data(object, "is_trading_suspended", GINT_TO_POINTER(sqlite3_column_int(stmt, 8)));
    g_object_set_data(object, "market", g_strdup((const gchar *)sqlite3_column_text(stmt, 9)));
    g_object_set_data(object, "market_display_name", g_strdup((const gchar *)sqlite3_column_text(stmt, 10)));

    gdouble *pip = g_new(gdouble, 1);
    *pip = sqlite3_column_double(stmt, 11);
    // gint degree = g_utf8_strchr(buffer, -1, '1') - g_utf8_strchr(buffer, -1, '.');
    g_object_set_data(object, "pip", pip);
    g_object_set_data(object, "quoted_currency_symbol", g_strdup((const gchar *)sqlite3_column_text(stmt, 12)));
    gdouble *spot = g_new(gdouble, 1);
    *spot = sqlite3_column_double(stmt, 13);
    g_object_set_data(object, "spot", spot);
    g_object_set_data(object, "spot_age", g_strdup((const gchar *)sqlite3_column_text(stmt, 14)));
    g_object_set_data(object, "spot_percentage_change", g_strdup((const gchar *)sqlite3_column_text(stmt, 15)));
    g_object_set_data(object, "spot_time", g_strdup((const gchar *)sqlite3_column_text(stmt, 16)));
    g_object_set_data(object, "subgroup", g_strdup((const gchar *)sqlite3_column_text(stmt, 17)));
    g_object_set_data(object, "subgroup_display_name", g_strdup((const gchar *)sqlite3_column_text(stmt, 18)));
    g_object_set_data(object, "submarket", g_strdup((const gchar *)sqlite3_column_text(stmt, 19)));
    g_object_set_data(object, "submarket_display_name", g_strdup((const gchar *)sqlite3_column_text(stmt, 20)));
    g_object_set_data(object, "symbol_type", g_strdup((const gchar *)sqlite3_column_text(stmt, 21)));
    sqlite3_finalize(stmt);

    g_object_set_data(task, "symbol", symbol);
}

void save_history(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    GObject *object = G_OBJECT(source);
    gpointer pointer = g_object_get_data(object, "symbol");
    const gchar *symbol = gtk_string_object_get_string(GTK_STRING_OBJECT(pointer));
    pointer = g_object_get_data(object, "timeframe");
    const gchar *home = gtk_string_object_get_string(GTK_STRING_OBJECT(object));
    const gchar *timeframe = gtk_string_object_get_string(GTK_STRING_OBJECT(pointer));
    gsize maxlength = strlen(home) + strlen(symbol) + 12;
    gchar *path = (gchar *)g_malloc0(maxlength);
    g_snprintf(path, maxlength, "%shistory/%s.db", home, symbol);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_clear_pointer(&path, g_free);

    gchar *suffix = "epoch INTEGER PRIMARY KEY, open INTEGER, close INTEGER, high INTEGER, low INTEGER);";
    maxlength = strlen(suffix) + strlen(timeframe) + 33;
    gchar *sql_symbol_history = (gchar *)g_malloc0(maxlength);
    g_snprintf(sql_symbol_history, maxlength, "CREATE TABLE IF NOT EXISTS \"%s\" (%s", timeframe, suffix);
    gchar *err_msg = NULL;
    sqlite3_exec(database, sql_symbol_history, 0, 0, &err_msg);
    g_clear_pointer(&sql_symbol_history, g_free);
    suffix = "epoch, open, close, high, low) VALUES (?, ?, ?, ?, ?);";
    maxlength = strlen(suffix) + strlen(timeframe) + 28;
    gchar *sql_insert = (gchar *)g_malloc0(maxlength);
    g_snprintf(sql_insert, maxlength, "INSERT OR IGNORE INTO \"%s\" (%s", timeframe, suffix);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);
    g_clear_pointer(&sql_insert, g_free);

    GListModel *model = G_LIST_MODEL(g_object_get_data(object, "candles"));
    gint count = g_list_model_get_n_items(model);

    gint start = count - GPOINTER_TO_INT(userdata);
    g_print("Saving %s history\n", symbol);
    for (gint position = start; start >= 0 && position < count; position++)
    {
        GObject *candle = g_list_model_get_item(model, position);
        GDateTime *epoch = (GDateTime *)g_object_get_data(candle, "epoch");
        gdouble *price = (gdouble *)g_object_get_data(candle, "price");
        sqlite3_bind_int64(stmt, 1, g_date_time_to_unix(epoch));
        sqlite3_bind_double(stmt, 2, price[0]);
        sqlite3_bind_double(stmt, 3, price[1]);
        sqlite3_bind_double(stmt, 4, price[2]);
        sqlite3_bind_double(stmt, 5, price[3]);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    g_print("%s history saved\n", symbol);
}

void update_active_symbols(GObject *object, JsonArray *list)
{
    const gchar *home = gtk_string_object_get_string(GTK_STRING_OBJECT(object));
    gsize maxlength = strlen(home) + 11;
    gchar *path = (gchar *)g_malloc0(maxlength);
    g_snprintf(path, maxlength, "%ssession.db", home);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_clear_pointer(&path, g_free);
    sqlite3_stmt *stmt;
    const gchar *sql_insert =
        "INSERT OR IGNORE INTO Symbols ("
        "symbol, display_name, allow_forward_starting, delay_amount, "
        "display_order, exchange_is_open, exchange_name, intraday_interval_minutes, "
        "is_trading_suspended, market, market_display_name, pip, "
        "quoted_currency_symbol, spot, spot_age, spot_percentage_change, "
        "spot_time, subgroup, subgroup_display_name, submarket, "
        "submarket_display_name, symbol_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_exec(database, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    for (gint index = 0; index < json_array_get_length(list); index++)
    {
        JsonObject *element = json_array_get_object_element(list, index);
        sqlite3_prepare_v2(database, sql_insert, -1, &stmt, NULL);

        sqlite3_bind_text(stmt, 1, json_object_get_string_member(element, "symbol"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, json_object_get_string_member(element, "display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, json_object_get_int_member(element, "allow_forward_starting"));
        sqlite3_bind_int(stmt, 4, json_object_get_int_member(element, "delay_amount"));
        sqlite3_bind_int(stmt, 5, json_object_get_int_member(element, "display_order"));
        sqlite3_bind_int(stmt, 6, json_object_get_int_member(element, "exchange_is_open"));
        sqlite3_bind_text(stmt, 7, json_object_get_string_member(element, "exchange_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 8, json_object_get_int_member(element, "intraday_interval_minutes"));
        sqlite3_bind_int(stmt, 9, json_object_get_int_member(element, "is_trading_suspended"));
        sqlite3_bind_text(stmt, 10, json_object_get_string_member(element, "market"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 11, json_object_get_string_member(element, "market_display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 12, json_object_get_double_member(element, "pip"));
        sqlite3_bind_text(stmt, 13, json_object_get_string_member(element, "quoted_currency_symbol"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 14, json_object_get_double_member(element, "spot"));
        sqlite3_bind_text(stmt, 15, json_object_get_string_member(element, "spot_age"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 16, json_object_get_string_member(element, "spot_percentage_change"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 17, json_object_get_string_member(element, "spot_time"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 18, json_object_get_string_member(element, "subgroup"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 19, json_object_get_string_member(element, "subgroup_display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 20, json_object_get_string_member(element, "submarket"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 21, json_object_get_string_member(element, "submarket_display_name"), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 22, json_object_get_string_member(element, "symbol_type"), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_exec(database, "COMMIT;", NULL, NULL, NULL);
    setup_symbol(object, database);
    sqlite3_close(database);
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
                                      "profile INTEGER,"
                                      "timeframe INTEGER"
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
    gchar *sql = "INSERT OR IGNORE INTO Session (symbol, profile, timeframe) VALUES ('BOOM1000', 0, 0);";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    sql = "INSERT OR IGNORE INTO Favourites (symbol, name) VALUES (?, ?);";
    sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
    gchar *list[4][2] = {{"BOOM1000", "Boom 1000 Index"},
                         {"CRASH1000", "Crash 1000 Index"},
                         {"R_75", "Volatility 75 Index"},
                         {"frxEURUSD", "EUR/USD"}};
    for (gint index = 0; index < 4; index++)
    {
        sqlite3_bind_text(stmt, 1, list[index][0], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, list[index][1], -1, SQLITE_STATIC);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void save_token_attributes(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    const gchar *home = gtk_string_object_get_string(GTK_STRING_OBJECT(source));
    gsize maxlength = strlen(home) + 11;
    gchar *path = (gchar *)g_malloc0(maxlength);
    g_snprintf(path, maxlength, "%ssession.db", home);
    sqlite3 *database;
    sqlite3_open(path, &database);
    g_clear_pointer(&path, g_free);
    const gchar *sql = "INSERT OR IGNORE INTO Accounts (account, currency) VALUES (?, ?);";
    GListModel *profile = G_LIST_MODEL(userdata);

    sqlite3_stmt *stmt;
    for (gint index = 0; index < g_list_model_get_n_items(profile); index++)
    {
        GtkStringObject *object = GTK_STRING_OBJECT(g_list_model_get_item(profile, index));
        const gchar *account = gtk_string_object_get_string(object);
        const gchar *currency = g_object_get_data(G_OBJECT(object), "currency");
        sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, account, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, currency, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(database);
};

void setup_user_interface(GObject *object)
{
    const gchar *home = gtk_string_object_get_string(GTK_STRING_OBJECT(object));
    gsize maxlength = strlen(home) + 8;
    gchar *path = (gchar *)g_malloc0(maxlength);
    g_snprintf(path, maxlength, "%shistory", home);
    GFile *file = g_file_new_for_path(path);
    g_clear_pointer(&path, g_free);
    maxlength = strlen(home) + 11;
    path = (gchar *)g_malloc0(maxlength);
    g_snprintf(path, maxlength, "%ssession.db", home);
    sqlite3 *database;
    gboolean exists = g_file_query_exists(g_file_new_for_path(path), NULL);
    GListStore *profile = g_object_get_data(object, "profile");

    const char *uri = "wss://ws.derivws.com/websockets/v3?app_id=66477";
    SoupMessage *message = soup_message_new(SOUP_METHOD_GET, uri);

    if (exists && sqlite3_open(path, &database) == SQLITE_OK)
    {
        gchar *sql = "SELECT account, currency FROM Accounts;";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(database, sql, -1, &stmt, NULL);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            gchar *account = (gchar *)sqlite3_column_text(stmt, 0);
            gchar *currency = g_strdup((gchar *)sqlite3_column_text(stmt, 1));
            GError *error = NULL;
            gchar *token = secret_password_lookup_sync(
                get_token_schema(), NULL, &error,
                "account", account,
                "currency", currency, NULL);
            GObject *string = G_OBJECT(gtk_string_object_new(account));
            g_object_set_data(string, "token", token);
            g_object_set_data(string, "currency", currency);
            g_list_store_append(profile, string);
        }
        sqlite3_finalize(stmt);
        setup_symbol(object, database);
        gint id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(profile), "id"));
        GObject *item = g_list_model_get_item(G_LIST_MODEL(profile), id);
        gchar *token = G_IS_OBJECT(item) ? g_object_get_data(item, "token") : NULL;

        gtk_window_present(GTK_WINDOW(g_object_get_data(object, "window")));

        if (token)
        {
            gsize maxlength = 8 + strlen(token);
            gchar *bearer = g_malloc0(maxlength);
            snprintf(bearer, maxlength, "Bearer %s", token);
            SoupMessageHeaders *headers = soup_message_get_request_headers(message);
            soup_message_headers_append(headers, "Authorization", bearer);
            g_clear_pointer(&bearer, g_free);
            present_actual_child(object);
        }
        else
        {
            setup_webview(object);
        }
    }
    else
    {
        g_file_make_directory_with_parents(file, NULL, NULL);
        if (sqlite3_open(path, &database) == SQLITE_OK)
        {
            create_default_tables(database);
            g_object_set_data(object, "fetch", GINT_TO_POINTER(1));
        }
    }
    sqlite3_close(database);
    SoupSession *session = soup_session_new();
    g_object_set_data(object, "session", session);
    soup_session_websocket_connect_async(
        session, message, NULL, NULL, G_PRIORITY_HIGH, NULL, connected, object);
    g_object_unref(message);
}