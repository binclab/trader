#include "main.h"
#include "cleanup.h"

static void synchronize_horizontal_scroll(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void synchronize_vertical_scroll(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void update_scale(GtkRange *range, BincData *bincdata)
{
    GtkAdjustment *adjustment = gtk_range_get_adjustment(range);
    int width = gtk_widget_get_width((GtkWidget *)range);
    int height = gtk_widget_get_height((GtkWidget *)range);
    double maximum = gtk_adjustment_get_upper(adjustment);
    double minimum = gtk_adjustment_get_lower(adjustment);
    double close_value = bincdata->price->close;

    gchar *buffer = g_strdup_printf("%.3f", close_value);
    if (bincdata->scalelabel != NULL)
        gtk_label_set_text(bincdata->scalelabel, buffer);
    g_free(buffer);

    double value = minimum + bincdata->data->space;

    if (close_value >= (maximum - bincdata->data->space))
    {
        int count = (int)ceil((close_value - maximum) / bincdata->data->space);
        for (int index = 0; index < count; index++)
        {
            height += MARKER_HEIGHT;
            gtk_widget_set_size_request((GtkWidget *)range, width, height);
            bincdata->data->maximum += bincdata->data->space;
            buffer = g_strdup_printf("%.3f", bincdata->data->maximum);
            gtk_scale_add_mark((GtkScale *)range, bincdata->data->maximum, GTK_POS_BOTTOM, buffer);
            // create_scale(bincdata->data, TRUE);
            g_free(buffer);
        }

        gtk_adjustment_set_upper(adjustment, bincdata->data->maximum);
    }
    else if (close_value <= (minimum + bincdata->data->space))
    {
        int count = (int)floor((minimum - close_value) / bincdata->data->space);
        for (int index = 0; index < count; index++)
        {
            height += MARKER_HEIGHT;
            gtk_widget_set_size_request((GtkWidget *)range, width, height);
            bincdata->data->minimum -= bincdata->data->space;
            buffer = g_strdup_printf("%.3f", bincdata->data->minimum);
            gtk_scale_add_mark((GtkScale *)range, bincdata->data->minimum, GTK_POS_BOTTOM, buffer);
            // create_scale(bincdata->data, FALSE);
            g_free(buffer);
        }
        gtk_adjustment_set_lower(adjustment, bincdata->data->minimum);
    }
}

static void listen_to_network(GNetworkMonitor *monitor, gboolean available, BincData *bincdata)
{
    gtk_toggle_button_set_active((GtkToggleButton *)bincdata->led, available);
    if (available && bincdata->connection != NULL)
    {
        soup_websocket_connection_send_text(bincdata->connection, request_ping());
    }
    else if (available)
    {
        g_task_run_in_thread(bincdata->task, setup_soup_session);
    }
}

void create_chart(BincData *bincdata)
{
    int count = g_list_model_get_n_items((GListModel *)bincdata->store);

    for (int position = 0; position < count; position++)
    {
        BincCandle *candle = g_list_model_get_item((GListModel *)bincdata->store, position);
        GtkFixed *fixed = add_candle(bincdata, candle);
        // double distance = 12 * (candle->price->open - candle->data->lowest);
        // gtk_fixed_put(fixed, bincdata->widget, 0, distance);
        if (position == count - 1)
        {
            bincdata->price = candle->price;
        }
        else
        {
            bincdata->widget = NULL;
        }
        /*if (
        {
            bincdata->widget = create_canvas(candle);
            gtk_box_append(bincdata->data->chart, bincdata->widget);
            // gtk_fixed_put(candle->data->fixed, bincdata->widget, candle->data->abscissa, candle->price->close - candle->data->baseline);
            // candle->data->abscissa += CANDLE_WIDTH;
        }
        else
        {

            GtkWidget * widget = create_canvas(candle);
            gtk_box_append(bincdata->data->chart, widget);

            // gtk_fixed_put(candle->data->fixed, create_canvas(candle), candle->data->abscissa, candle->price->close - candle->data->baseline);
            // candle->data->abscissa += CANDLE_WIDTH;
        }*/
        // free(candles[position]);
    }
    GtkAdjustment *adjustment = gtk_range_get_adjustment(bincdata->data->rangescale);
    CandleStatistics *stat = bincdata->stat;
    gtk_adjustment_set_upper(adjustment, stat->highest);
    gtk_adjustment_set_lower(adjustment, stat->lowest);
    bincdata->data->space = (stat->highest - stat->lowest) / 20;
    // data->height = bincdata->rectangle->height / 20;
    bincdata->data->minimum = stat->lowest;
    bincdata->data->maximum = stat->highest;
    for (gdouble index = stat->lowest; index < stat->highest; index += bincdata->data->space)
    {
        /*char *buffer = g_strdup_printf("%.3f", index);
        gtk_scale_add_mark((GtkScale *)bincdata->data->rangescale, index, GTK_POS_BOTTOM, buffer);
        bincdata->data->maximum = index;
        g_free(buffer);*/
        create_scale(bincdata->data, index, 2);
    }
    //g_signal_connect(bincdata->data->rangescale, "value-changed", (GCallback)update_scale, bincdata);
}

/*
static void activate_application(GtkApplication *app, gchar *home)
{
    GtkWidget *window = binc_window_new(app);
    GtkNative *native = gtk_widget_get_native(window);
    // create_symbol_database(home);
    //  GdkMonitor *monitor = g_list_model_get_item(gdk_display_get_monitors(display), 0);
    setup_window((BincWindow *)window, home);
    setup_header();
    setup_navigation(((BincWindow *)window)->height);
    // setup_children((BincWindow *)window);
    setup_signals((BincWindow *)window);
    add_children();
    gtk_window_set_child((GtkWindow *)window, layout1);
    gtk_window_present((GtkWindow *)window);
    g_idle_add_once((GSourceOnceFunc)setup_soup_session, window);
    // g_signal_connect(window, "notify::focus-visible", (GCallback)notify_window_visible, home);
}*/

static void setup_children(BincData *bincdata)
{
    GtkWidget *pricescale = gtk_scale_new(GTK_ORIENTATION_VERTICAL, NULL);
    GtkWidget *scalebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *timebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *chartbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *cartbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *growbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scalescroll = gtk_scrolled_window_new();
    GtkWidget *timescroll = gtk_scrolled_window_new();
    GtkWidget *cartscroll = gtk_scrolled_window_new();
    GtkWidget *scaleport = gtk_viewport_new(NULL, NULL);
    GtkWidget *timeport = gtk_viewport_new(
        gtk_adjustment_new(0, 0, 1080, 0.01, 1, CANDLE_WIDTH * 5),
        gtk_adjustment_new(0, 0, 48, 0, 0, 0));
    GtkWidget *cartport = gtk_viewport_new(
        gtk_adjustment_new(0, 0, 1080, 0.01, 1, CANDLE_WIDTH * 5),
        gtk_adjustment_new(0, 0, CANDLE_HEIGHT, 0.01, 1, 12));

    GtkWidget *scalelabel = gtk_label_new("Info");
    GtkWidget *b1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *b2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *b3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *b4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *b5 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *b6 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *startchild = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *endchild = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *t1 = gtk_button_new();
    GtkWidget *t2 = gtk_button_new();
    GtkWidget *navigation = gtk_button_new();
    GtkWidget *t4 = gtk_button_new();

    GtkWidget *fixed = gtk_fixed_new();
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    bincdata->data->paned = (GtkPaned *)paned;
    bincdata->data->box = (GtkBox *)timebox;
    bincdata->data->chart = (GtkBox *)chartbox;
    bincdata->data->chartgrow = (GtkBox *)growbox;
    bincdata->data->rangescale = (GtkRange *)pricescale;
    bincdata->scalelabel = (GtkLabel *)scalelabel;
    bincdata->timeport = (GtkViewport *)timeport;
    bincdata->scrollinfo = gtk_scroll_info_new();
    bincdata->scalescroll = (GtkScrolledWindow *)scalescroll;
    bincdata->chartgravity = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    bincdata->timegravity = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    // bincdata->adjustment = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)timescroll);
    bincdata->adjustment = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow *)timescroll);

    setup_cartesian((GtkBox *)cartbox, bincdata);
    gtk_overlay_add_overlay(bincdata->data->overlay, chartbox);

    // gtk_widget_set_valign(chartbox, GTK_ALIGN_CENTER);
    // gtk_widget_set_valign(scalebox, GTK_ALIGN_CENTER);
    // gtk_widget_set_valign(fixed, GTK_ALIGN_CENTER);
    //  gtk_widget_set_size_request(fixed, -1, bincdata->rectangle->height *3);
    ///

    gtk_widget_set_size_request(bincdata->chartgravity, 48, CANDLE_HEIGHT);
    gtk_widget_set_size_request(bincdata->timegravity, 48, 48);
    gtk_widget_set_size_request(scalebox, 138, CANDLE_HEIGHT);
    gtk_widget_set_size_request(pricescale, 16, CANDLE_HEIGHT);
    gtk_widget_set_size_request(paned, 1080, CANDLE_HEIGHT);
    gtk_widget_set_size_request(cartscroll, 1080, CANDLE_HEIGHT - CONTENT_HEIGHT);
    gtk_widget_set_size_request(scalelabel, 138, 48);
    gtk_widget_set_size_request(t2, -1, 200);
    gtk_widget_set_size_request(navigation, 200, -1);

    /*gtk_widget_set_size_request(chartscroll, width, height);
    gtk_widget_set_size_request(timescroll, width, 48);
    gtk_widget_set_size_request(scalescroll, 108, height);*/

    // gtk_widget_set_size_request(paned, 1080, 472);

    // gtk_widget_set_hexpand(chartport, TRUE);
    gtk_widget_set_hexpand(timeport, TRUE);
    gtk_widget_set_hexpand(b3, TRUE);
    gtk_widget_set_hexpand(cartport, TRUE);
    // gtk_widget_set_vexpand(chartport, TRUE);
    // gtk_widget_set_vexpand(timeport, TRUE);
    gtk_widget_set_vexpand(scaleport, TRUE);
    gtk_widget_set_vexpand(navigation, TRUE);

    // gtk_widget_set_margin_end(chartport, 138);

    gtk_widget_set_valign(pricescale, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(scalescroll, GTK_ALIGN_END);
    // gtk_widget_set_valign(paned, GTK_ALIGN_);
    // gtk_widget_set_halign(paned, GTK_ALIGN_END);

    gtk_widget_set_name(timeport, "timeport");

    // gtk_widget_add_css_class(growboxscale, "marker");

    gtk_widget_set_sensitive(pricescale, FALSE);

    gtk_viewport_set_scroll_to_focus(GTK_VIEWPORT(timeport), TRUE);
    gtk_scroll_info_set_enable_horizontal(bincdata->scrollinfo, TRUE);
    gtk_range_set_inverted(bincdata->data->rangescale, TRUE);

    /*gtk_scrolled_window_set_policy((GtkScrolledWindow *)scalescroll, GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)timescroll, GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)chartscroll, GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);*/

    gtk_scrolled_window_set_policy((GtkScrolledWindow *)scalescroll, GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)timescroll, GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)cartscroll, GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);

    gtk_scrolled_window_set_child((GtkScrolledWindow *)scalescroll, scaleport);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)timescroll, timeport);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)cartscroll, cartport);

    gtk_paned_set_start_child((GtkPaned *)paned, startchild);
    gtk_paned_set_end_child((GtkPaned *)paned, endchild);

    // gtk_fixed_put((GtkFixed *)fixed, growbox, 0, 0);
    // gtk_fixed_put((GtkFixed *)fixed, chartoverlay, 0, 0);
    // gtk_overlay_set_child((GtkOverlay *)cartoverlay, cartscroll);
    // gtk_overlay_add_overlay((GtkOverlay *)cartoverlay, paned);
    // gtk_overlay_add_overlay((GtkOverlay *)cartoverlay, scalescroll);

    gtk_viewport_set_child((GtkViewport *)scaleport, scalebox);
    gtk_viewport_set_child((GtkViewport *)cartport, cartbox);
    gtk_viewport_set_child((GtkViewport *)timeport, b6);

    // gtk_box_append((GtkBox *)growbox, chartbox);
    gtk_box_append((GtkBox *)scalebox, pricescale);
    // gtk_box_append((GtkBox *)scalebox, growboxscale);
    gtk_box_append((GtkBox *)b6, timebox);
    gtk_box_append((GtkBox *)b6, bincdata->timegravity);
    // gtk_box_append((GtkBox *)b5, fixed);
    gtk_box_append((GtkBox *)b5, bincdata->chartgravity);
    // gtk_box_append((GtkBox *)b4, chartscroll);
    // gtk_box_append((GtkBox *)b4, scalescroll);
    gtk_box_append((GtkBox *)b3, timescroll);
    gtk_box_append((GtkBox *)b3, scalelabel);
    gtk_box_append((GtkBox *)b2, cartscroll);
    gtk_box_append((GtkBox *)b2, b3);
    gtk_box_append((GtkBox *)b1, navigation);
    gtk_box_append((GtkBox *)b1, b2);

    gtk_box_append((GtkBox *)bincdata->child, t1);
    gtk_box_append((GtkBox *)bincdata->child, b1);
    gtk_box_append((GtkBox *)bincdata->child, t2);

    GtkAdjustment *charthadjustment = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow *)cartscroll);
    GtkAdjustment *chartvadjustment = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow *)cartscroll);
    GtkAdjustment *timeadjustment = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow *)timescroll);
    GtkAdjustment *scaleadjustment = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow *)scalescroll);
    g_signal_connect(charthadjustment, "value-changed", (GCallback)synchronize_horizontal_scroll, timeadjustment);
    g_signal_connect(timeadjustment, "value-changed", (GCallback)synchronize_horizontal_scroll, charthadjustment);
    g_signal_connect(scaleadjustment, "value-changed", (GCallback)synchronize_vertical_scroll, chartvadjustment);
    g_signal_connect(chartvadjustment, "value-changed", (GCallback)synchronize_vertical_scroll, scaleadjustment);
    // g_signal_connect(networkmonitor, "network-changed", (GCallback)listen_to_network, window);
}

static const SecretSchema *get_token_schema(void)
{
    static const SecretSchema schema = {
        "com.binclab.Trader",
        SECRET_SCHEMA_NONE,
        {{"account", SECRET_SCHEMA_ATTRIBUTE_STRING},
         {"currency", SECRET_SCHEMA_ATTRIBUTE_STRING},
         {NULL, 0}}};
    return &schema;
};

static void clear_specific_account()
{
    GError *error = NULL;

    secret_password_clear_sync(get_token_schema(), NULL, &error,
                               "account", "your_account1_value",
                               "currency", "your_token1_value", NULL);

    if (error != NULL)
    {
        fprintf(stderr, "Error clearing data: %s\n", error->message);
        g_error_free(error);
    }
    else
    {
        printf("Specific data cleared successfully.\n");
    }
}

static void present_actual_child(BincData *bincdata)
{
    // gtk_window_maximize(bincdata->window);
    // gtk_window_set_resizable(bincdata->window, FALSE);
    gtk_window_set_child(bincdata->window, bincdata->child);
    g_task_run_in_thread(bincdata->task, setup_soup_session);
}

static void store_token(GObject *source, GAsyncResult *result, gpointer data)
{
    BincData *bincdata = (BincData *)data;
    GError *error = NULL;
    if (error != NULL)
    {
        fprintf(stderr, "Error storing data: %s\n", error->message);
        g_error_free(error);
    }
    else
    {
        present_actual_child(bincdata);
    }
}

static void load_changed(WebKitWebView *webview, WebKitLoadEvent event, gpointer data)
{
    BincData *bincdata = (BincData *)data;
    switch (event)
    {
    case WEBKIT_LOAD_STARTED:
        break;
    case WEBKIT_LOAD_REDIRECTED:
        char *url = (char *)webkit_web_view_get_uri(webview);
        if (strstr(url, "https://www.binclab.com/deriv?"))
        {
            char *param = strtok(g_strdup(strstr(url, "acct1")), "&");
            for (int i = 0; param != NULL; i++)
            {
                char *value = strchr(param, '=');
                value++;
                bincdata->account[i]->account = g_strdup(value);

                param = strtok(NULL, "&");
                value = strchr(param, '=');
                value++;
                bincdata->account[i]->token = g_strdup(value);

                param = strtok(NULL, "&");
                value = strchr(param, '=');
                value++;
                bincdata->account[i]->currency = g_strdup(value);
                param = strtok(NULL, "&");

                secret_password_store(get_token_schema(), SECRET_COLLECTION_DEFAULT,
                                      "Binc Trader Token", bincdata->account[i]->token, NULL,
                                      store_token, bincdata,
                                      "account", bincdata->account[i]->account,
                                      "currency", bincdata->account[i]->currency,
                                      NULL);
            }
            g_task_run_in_thread(bincdata->task, save_token_attributes);
            present_actual_child(bincdata);
        }
        break;
    case WEBKIT_LOAD_COMMITTED:
        break;
    case WEBKIT_LOAD_FINISHED:
        break;
    }
}

static void setup_webview(BincData *bincdata)
{
    bincdata->webview = webkit_web_view_new();
    WebKitNetworkSession *session =
        webkit_web_view_get_network_session((WebKitWebView *)bincdata->webview);
    WebKitCookieManager *cookiejar =
        webkit_network_session_get_cookie_manager(session);
    char *storage = (char *)malloc(strlen(bincdata->home) + 10);
    sprintf(storage, "%ssession.db", bincdata->home);
    gchar *uri = "https://oauth.deriv.com/oauth2/authorize?app_id=66477";
    webkit_cookie_manager_set_persistent_storage(cookiejar, storage, 1);
    webkit_web_view_load_uri((WebKitWebView *)bincdata->webview, uri);
    g_signal_connect(bincdata->webview, "load-changed", (GCallback)load_changed, bincdata);
}

static void on_token_lookup(GObject *source, GAsyncResult *result, gpointer data)
{
    GError *error = NULL;
    BincData *bincdata = (BincData *)data;

    if (error != NULL)
    {
        printf("handle the failure here */\n");
        g_error_free(error);
    }
    else if (bincdata->account[0]->token == NULL)
    {
        bincdata->account[0]->token = secret_password_lookup_finish(result, &error);
    }
    else if (bincdata->account[1]->token == NULL)
    {
        bincdata->account[1]->token = secret_password_lookup_finish(result, &error);
    }
    else if (bincdata->account[2]->token == NULL)
    {
        bincdata->account[2]->token = secret_password_lookup_finish(result, &error);
        present_actual_child(bincdata);
    }
    else
    {
        gtk_window_set_child(bincdata->window, bincdata->webview);
    }
}

static void setup_accounts(BincData *bincdata)
{
    if (get_token_attributes(bincdata))
    {
        for (int index = 0; index < 3; index++)
        {
            secret_password_lookup(get_token_schema(), NULL, on_token_lookup, bincdata,
                                   "account", bincdata->account[index]->account,
                                   "currency", bincdata->account[index]->currency, NULL);
        }
    }
}

static void activate(GtkApplication *application, BincData *bincdata)
{
    const char *styles = "/com/binclab/trader/css/style.css";
    display = gdk_display_get_default();
    networkmonitor = g_network_monitor_get_default();
    provider = (GtkStyleProvider *)gtk_css_provider_new();
    theme = gtk_icon_theme_get_for_display(display);
    bincdata->window = (GtkWindow *)gtk_application_window_new(application);
    bincdata->child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_icon_theme_add_resource_path(theme, "/com/binclab/trader/icons/96x96");
    gtk_css_provider_load_from_resource((GtkCssProvider *)provider, styles);
    gtk_style_context_add_provider_for_display(display, provider, 600);

    gtk_widget_set_size_request((GtkWidget *)bincdata->window, 1280, CANDLE_HEIGHT);
    gtk_window_set_title(bincdata->window, "Binc Trader");
    gtk_window_present(bincdata->window);

    surface = gtk_native_get_surface(gtk_widget_get_native((GtkWidget *)bincdata->window));
    GdkMonitor *monitor = gdk_display_get_monitor_at_surface(display, surface);
    gdk_monitor_get_geometry(monitor, bincdata->rectangle);

    setup_webview(bincdata);
    setup_children(bincdata);
    setup_accounts(bincdata);

    g_signal_connect(bincdata->window, "close-request", (GCallback)close_request, bincdata);
}

/*static void activate(GtkWindow *logowindow, BincData *bincdata)
{
    GtkApplication *application = gtk_window_get_application(logowindow);
    bincdata->rectangle->width = ((BincWindow *)logowindow)->width;
    bincdata->rectangle->height = ((BincWindow *)logowindow)->height;
    bincdata->window = (GtkWindow *)gtk_application_window_new(application);
    bincdata->child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request((GtkWidget*)bincdata->window, 1280, CANDLE_HEIGHT);
    setup_children((GtkBox *)bincdata->child, bincdata);
    gtk_window_set_title(bincdata->window, "Binc Terminal");
    gtk_window_present(bincdata->window);
    setup_accounts(bincdata);
}*/

/*
static const char *get_shell_type()
{
    const char *shell = getenv("XDG_CURRENT_DESKTOP");
    if (shell)
    {styles
        return shell;
    }
    shell = getenv("DESKTOP_SESSION");
    if (shell)
    {
        return shell;
    }
    return "Unknown";
}*/

/*
static void showlogo(GtkApplication *application, BincData *bincdata)
{
    GtkWindow *logowindow = (GtkWindow *)binc_window_new(application);
    const char *styles = "/com/binclab/terminal/css/style.css";
    display = gdk_display_get_default();
    networkmonitor = g_network_monitor_get_default();
    provider = (GtkStyleProvider *)gtk_css_provider_new();
    theme = gtk_icon_theme_get_for_display(display);
    gtk_icon_theme_add_resource_path(theme, "/com/binclab/terminal/icons/96x96");
    gtk_css_provider_load_from_resource((GtkCssProvider *)provider, styles);
    gtk_style_context_add_provider_for_display(display, provider, 600);
    setup_webview(bincdata);
    gtk_window_present(logowindow);
    g_idle_add_once((GSourceOnceFunc)gtk_window_maximize, logowindow);
}*/

int main(int argc, char *argv[])
{
    // setenv("GDK_BACKEND", "wayland", 1);
    BincData *bincdata = g_object_new(BINC_TYPE_DATA, NULL);
    bincdata->home = g_strdup_printf("%s/binctrader/", g_get_user_data_dir());
    GtkApplication *app = gtk_application_new("com.binclab.trader", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", (GCallback)activate, bincdata);
    // g_signal_connect(app, "shutdown", (GCallback)shutdown, bincdata);
    g_application_run((GApplication *)app, argc, argv);
    g_object_unref(bincdata);
    g_object_unref(app);
    return 0;
}

/*clock_t start_time, end_time;
start_time = clock();
end_time = clock();
clock_t time1 = start_time - end_time;
start_time = clock();
end_time = clock();
printf("time %li, %li\n", time1, start_time - end_time);*/