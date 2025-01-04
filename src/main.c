#include "main.h"
#include <pthread.h>

static void synchronize_horizontal_scroll(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void synchronize_vertical_scroll(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void update_scale(GtkRange *range, BincWindow *window)
{
    GtkAdjustment *adjustment = gtk_range_get_adjustment(range);
    double upper = gtk_adjustment_get_upper(adjustment);
    double lower = gtk_adjustment_get_lower(adjustment);
    double close_value = window->price->close;

    char buffer[10];

    snprintf(buffer, 9, "%.3f", close_value);
    if (scaleinfo != NULL)
        gtk_label_set_text((GtkLabel *)scaleinfo, buffer);
    memset(buffer, 0, 10);

    if (close_value >= (upper - window->model->data->space))
    {
        double mark = upper + window->model->data->space;
        gtk_adjustment_set_upper(adjustment, mark);
        snprintf(buffer, 10, "%.3f", mark);
        // gtk_box_prepend(window->scalebox, create_scale(mark));
        gtk_scale_add_mark((GtkScale *)scale, mark, GTK_POS_BOTTOM, buffer);
        // gtk_widget_set_size_request(fixed, window->width + 108, gtk_widget_get_height(fixed) + window->model->data->height);
    }
    else if (close_value <= (lower + window->model->data->space))
    {
        double mark = lower - window->model->data->space;
        gtk_adjustment_set_lower(adjustment, mark);
        snprintf(buffer, 10, "%.3f", mark);
        // gtk_box_append(window->scalebox, create_scale(mark));
        gtk_scale_add_mark((GtkScale *)scale, mark, GTK_POS_BOTTOM, buffer);
        // gtk_widget_set_size_request(fixed, window->width + 108, gtk_widget_get_height(fixed) + window->model->data->height);
    }
}

static void setup_header()
{
    header1 = gtk_button_new();
    gtk_widget_set_size_request(header1, -1, 48);
    gtk_box_append((GtkBox *)layout1, header1);
}

static void setup_navigation(int height)
{
    navigation = gtk_button_new();
    gtk_widget_set_size_request(navigation, 200, height);
    gtk_box_append((GtkBox *)layout2, navigation);
}

static void setup_window(BincWindow *window, gchar *home)
{
    GtkAdjustment *vertical = gtk_adjustment_new(window->height, 0, window->height, 0.01, 0.1, 10);
    GtkAdjustment *horizontal = gtk_adjustment_new(window->width, 0, window->width, 48, 48, 48);
    window->home = home;
    layout1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    layout2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    layout3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    canvasbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    // scalebox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    layout4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    scaleinfo = gtk_label_new(NULL);
    chart = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    timechart = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    fixed = gtk_fixed_new();
    infoarea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    infobox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    termspace = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    termframe = gtk_frame_new("Infomation");
    terminfo = gtk_label_new("No bytes used");
    termbar = gtk_action_bar_new();
    conled = gtk_toggle_button_new();
    termbar = gtk_action_bar_new();
    accountbook = gtk_notebook_new();
    scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.0, window->height, 0.001);
    chartport = gtk_viewport_new(horizontal, vertical);
    timeport = gtk_viewport_new(horizontal, NULL);
    scaleport = gtk_viewport_new(NULL, vertical);
    chartscroll = gtk_scrolled_window_new();
    timescroll = gtk_scrolled_window_new();
    scalescroll = gtk_scrolled_window_new();

    // chartpane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    // timepane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    window->scrollinfo = gtk_scroll_info_new();
    window->timeport = (GtkViewport *)timeport;
    window->scale = (GtkRange *)scale;
    // window->scalebox = (GtkBox *)scalebox;
    window->infolabel = (GtkLabel *)terminfo;
    window->led = (GtkToggleButton *)conled;
    window->chart = (GtkBox *)chart;

    window->model->timeframe = "1 MINUTE";
    window->model->home = window->home;
    window->model->data->box = (GtkBox *)timechart;
}

static void setup_children2(BincWindow *window)
{
    gtk_widget_set_size_request(timeport, -1, 48);
    gtk_widget_set_size_request(scaleinfo, 108, 48);
    gtk_widget_set_size_request(termspace, -1, -1);
    gtk_widget_set_size_request(termframe, 200, 200);
    gtk_widget_set_size_request(scale, 108, window->height - 48);
    // gtk_widget_set_size_request(scalebox, 92, window->height - 48);
    gtk_widget_set_size_request(layout3, window->width + 108, 48);
    gtk_widget_set_size_request(layout4, window->width + 108, window->height - 48);
    gtk_widget_set_size_request(chartscroll, window->width, window->height);
    gtk_widget_set_size_request(timescroll, window->width, 48);
    gtk_widget_set_size_request(scalescroll, window->width + 108, window->height - 48);
    gtk_widget_set_size_request(chart, window->width, window->height);
    gtk_widget_set_size_request(fixed, window->width, window->height);
    // gtk_widget_set_size_request(chartpane, window->width + 108, window->height);
    // gtk_widget_set_size_request(timepane, window->width + 108, 48);
    gtk_widget_set_size_request(canvasbox, window->width, window->height);
    // gtk_widget_set_margin_top(scalebox, 4);
    // gtk_widget_set_margin_bottom(scalebox, 4);
    // gtk_widget_set_margin_top(chart, 4);
    // gtk_widget_set_margin_bottom(chart, 4);
    gtk_widget_set_vexpand(scale, TRUE);
    gtk_widget_set_vexpand(layout2, TRUE);
    gtk_widget_set_vexpand(chart, TRUE);
    gtk_widget_set_vexpand(termspace, TRUE);
    gtk_widget_set_hexpand(chart, TRUE);
    // gtk_widget_set_hexpand(timescroll, TRUE);
    gtk_widget_set_name(chartport, "chartport");
    gtk_widget_set_name(timeport, "timeport");
    gtk_widget_set_name(conled, "led");

    gtk_widget_set_halign(chart, GTK_ALIGN_END);
    gtk_widget_set_halign(timechart, GTK_ALIGN_END);
    gtk_widget_set_halign(scaleinfo, GTK_ALIGN_CENTER);

    gtk_frame_set_label_align((GtkFrame *)termframe, 0.5);

    gtk_scale_set_has_origin((GtkScale *)scale, TRUE);
    gtk_range_set_inverted((GtkRange *)scale, TRUE);
    // gtk_scale_set_draw_value((GtkScale *)scale, TRUE);
    gtk_scale_set_value_pos((GtkScale *)scale, GTK_POS_TOP);
    gtk_widget_set_sensitive(scale, FALSE);
    gtk_widget_add_css_class(conled, "circular");

    gtk_label_set_justify((GtkLabel *)scaleinfo, GTK_JUSTIFY_CENTER);
    gtk_notebook_set_tab_pos((GtkNotebook *)accountbook, GTK_POS_LEFT);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)chartscroll, GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)timescroll, GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)scalescroll, GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
    // gtk_scrolled_window_set_placement((GtkScrolledWindow *)chartscroll, GTK_CORNER_TOP_RIGHT);

    gtk_scroll_info_set_enable_horizontal(((BincWindow *)window)->scrollinfo, TRUE);
}

static void listen_to_network(GNetworkMonitor *monitor, gboolean available, BincWindow *window)
{
    gtk_toggle_button_set_active((GtkToggleButton *)conled, available);
    if (available && window->connection != NULL)
    {
        soup_websocket_connection_send_text(window->connection, request_ping());
    }
    else if (available)
    {
        setup_soup_session((void *)window);
    }
}

static void setup_signals(BincWindow *window)
{
    GtkAdjustment *charthadjustment = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow *)chartscroll);
    GtkAdjustment *chartvadjustment = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow *)chartscroll);
    GtkAdjustment *timeadjustment = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow *)timescroll);
    GtkAdjustment *scaleadjustment = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow *)scalescroll);
    g_signal_connect(charthadjustment, "value-changed", (GCallback)synchronize_horizontal_scroll, timeadjustment);
    g_signal_connect(timeadjustment, "value-changed", (GCallback)synchronize_horizontal_scroll, charthadjustment);
    g_signal_connect(scaleadjustment, "value-changed", (GCallback)synchronize_vertical_scroll, chartvadjustment);
    g_signal_connect(chartvadjustment, "value-changed", (GCallback)synchronize_vertical_scroll, scaleadjustment);
    g_signal_connect(networkmonitor, "network-changed", (GCallback)listen_to_network, window);
}

static void add_children()
{
    gtk_fixed_put((GtkFixed *)fixed, chart, 0, 0);
    gtk_viewport_set_child((GtkViewport *)chartport, fixed);
    gtk_viewport_set_child((GtkViewport *)timeport, timechart);
    gtk_viewport_set_child((GtkViewport *)scaleport, layout4);

    gtk_scrolled_window_set_child((GtkScrolledWindow *)chartscroll, chartport);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)timescroll, timeport);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)scalescroll, scaleport);

    gtk_action_bar_pack_start((GtkActionBar *)termbar, conled);
    gtk_box_append((GtkBox *)infobox, terminfo);
    gtk_box_append((GtkBox *)infobox, termspace);
    gtk_box_append((GtkBox *)infobox, termbar);
    gtk_frame_set_child((GtkFrame *)termframe, infobox);

    // gtk_paned_set_start_child((GtkPaned *)chartpane, chartscroll);
    // gtk_paned_set_end_child((GtkPaned *)chartpane, scalescroll);
    // gtk_paned_set_start_child((GtkPaned *)timepane, timescroll);
    // gtk_paned_set_end_child((GtkPaned *)timepane, scaleinfo);

    gtk_box_append((GtkBox *)infoarea, termframe);
    gtk_box_append((GtkBox *)infoarea, accountbook);
    gtk_box_append((GtkBox *)layout4, chartscroll);
    gtk_box_append((GtkBox *)layout4, scale);
    // gtk_box_append((GtkBox *)layout4, scalebox);
    // gtk_box_append((GtkBox *)canvasbox, chartpane);
    gtk_box_append((GtkBox *)layout3, timescroll);
    gtk_box_append((GtkBox *)layout3, scaleinfo);
    gtk_box_append((GtkBox *)canvasbox, scalescroll);
    gtk_box_append((GtkBox *)canvasbox, layout3);
    gtk_box_append((GtkBox *)layout2, canvasbox);
    gtk_box_append((GtkBox *)layout1, layout2);
    gtk_box_append((GtkBox *)layout1, infoarea);
}

void create_chart(BincWindow *window)
{
    int count = g_list_model_get_n_items((GListModel *)window->model);

    for (int position = 0; position < count; position++)
    {
        Candle *candle = g_list_model_get_item((GListModel *)window->model, position);
        if (position == count - 1)
        {
            window->widget = create_canvas(candle);
            gtk_box_append((GtkBox *)chart, window->widget);
        }
        else
        {
            gtk_box_append((GtkBox *)chart, create_canvas(candle));
        }
        // free(candles[position]);
    }
    GtkAdjustment *adjustment = gtk_range_get_adjustment((GtkRange *)scale);
    CandleData *data = window->model->data;
    gtk_adjustment_set_upper(adjustment, data->highest);
    gtk_adjustment_set_lower(adjustment, data->lowest);
    data->space = (data->highest - data->lowest) / 20;
    data->height = window->height / 20;
    for (double index = data->lowest; index < data->highest; index += data->space)
    {
        char buffer[10];
        snprintf(buffer, 9, "%.3f", index);
        gtk_scale_add_mark((GtkScale *)scale, index, GTK_POS_BOTTOM, buffer);
        // gtk_box_prepend(window->scalebox, create_scale(index));
    }
    g_signal_connect(scale, "value-changed", (GCallback)update_scale, window);
}

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
}

static void setup_children(GtkBox *parent, int height)
{
    GtkWidget *s1 = gtk_scale_new(GTK_ORIENTATION_VERTICAL, NULL);
    GtkWidget *b5 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *b6 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *b2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *sw1 = gtk_scrolled_window_new();
    GtkWidget *sw2 = gtk_scrolled_window_new();
    GtkWidget *sw3 = gtk_scrolled_window_new();
    GtkWidget *v1 = gtk_viewport_new(NULL, NULL);
    GtkWidget *v2 = gtk_viewport_new(NULL, NULL);
    GtkWidget *v3 = gtk_viewport_new(NULL, NULL);

    GtkWidget *l1 = gtk_label_new("Info");
    GtkWidget *b3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *b4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *t1 = gtk_button_new();
    GtkWidget *t2 = gtk_button_new();
    GtkWidget *t3 = gtk_button_new();
    GtkWidget *t4 = gtk_button_new();

    gtk_widget_set_valign(b6, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(b6, 1, height);
    gtk_widget_set_size_request(s1, 107, -1);
    gtk_widget_set_size_request(l1, 108, 48);

    gtk_widget_set_hexpand(v3, TRUE);
    gtk_widget_set_vexpand(v3, TRUE);
    gtk_widget_set_vexpand(v1, TRUE);
    gtk_widget_set_hexpand(v1, TRUE);
    gtk_widget_set_hexpand(v2, TRUE);

    gtk_widget_set_name(v3, "chartport");
    gtk_widget_set_name(v2, "timeport");

    gtk_widget_add_css_class(b6, "marker");

    gtk_scrolled_window_set_policy((GtkScrolledWindow *)sw1, GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)sw2, GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)sw3, GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);

    gtk_scrolled_window_set_child((GtkScrolledWindow *)sw1, v1);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)sw2, v2);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)sw3, v3);

    gtk_viewport_set_child((GtkViewport *)v1, b5);
    gtk_box_append((GtkBox *)b5, sw3);
    gtk_box_append((GtkBox *)b5, s1);
    gtk_box_append((GtkBox *)b5, b6);
    gtk_box_append((GtkBox *)b4, sw2);
    gtk_box_append((GtkBox *)b4, l1);
    gtk_box_append((GtkBox *)b3, sw1);
    gtk_box_append((GtkBox *)b3, b4);
    gtk_box_append((GtkBox *)b2, t3);
    gtk_box_append((GtkBox *)b2, b3);

    gtk_widget_set_vexpand(t3, TRUE);
    gtk_widget_set_size_request(t2, -1, 200);
    gtk_widget_set_size_request(t3, 200, -1);
    gtk_box_append((GtkBox *)parent, t1);
    gtk_box_append((GtkBox *)parent, b2);
    gtk_box_append((GtkBox *)parent, t2);
}

/*

    gtk_scrolled_window_set_child((GtkScrolledWindow *)sw1, v1);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)sw2, v2);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)sw3, v3);

    gtk_viewport_set_child((GtkViewport *)v1, b5);

    gtk_box_append((GtkBox *)b5, sw3);
    gtk_box_append((GtkBox *)b5, s1);
    gtk_box_append((GtkBox *)b5, b6);
    gtk_box_append((GtkBox *)b4, sw2);
    gtk_box_append((GtkBox *)b4, l1);
    gtk_box_append((GtkBox *)b3, sw1);
    gtk_box_append((GtkBox *)b3, b4);
    gtk_box_append((GtkBox *)b2, t3);
    gtk_box_append((GtkBox *)b2, b3);
    gtk_box_append((GtkBox *)b1, t1);
    gtk_box_append((GtkBox *)b1, b2);
    gtk_box_append((GtkBox *)b1, t2);
*/

static void activate(GtkApplication *app, DataObject *object)
{
    const char *styles = "/com/binclab/terminal/css/style.css";
    display = gdk_display_get_default();
    networkmonitor = g_network_monitor_get_default();
    provider = (GtkStyleProvider *)gtk_css_provider_new();
    theme = gtk_icon_theme_get_for_display(display);
    GListModel *model = gdk_display_get_monitors(display);
    gdk_monitor_get_geometry(g_list_model_get_item(model, 0), object->rectangle);
    gtk_icon_theme_add_resource_path(theme, "/com/binclab/terminal/icons/96x96");
    gtk_css_provider_load_from_resource((GtkCssProvider *)provider, styles);
    gtk_style_context_add_provider_for_display(display, provider, 600);

    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    setup_children((GtkBox *)child, object->rectangle->height * 3);
    gtk_widget_set_size_request(window, 1280, 720);
    gtk_window_set_title((GtkWindow *)window, "Binc Terminal");
    gtk_window_set_child((GtkWindow *)window, child);
    gtk_window_present((GtkWindow *)window);
    pthread_t thread;
    pthread_create(&thread, NULL, setup_soup_session, (void *)object);
    pthread_detach(thread);
    // g_idle_add_once((GSourceOnceFunc)setup_content, &data);
}

/*
static const char *get_shell_type()
{
    const char *shell = getenv("XDG_CURRENT_DESKTOP");
    if (shell)
    {
        return shell;
    }
    shell = getenv("DESKTOP_SESSION");
    if (shell)
    {
        return shell;
    }
    return "Unknown";
}*/

int main(int argc, char *argv[])
{
    // setenv("GDK_BACKEND", "wayland", 1);
    DataObject *object = (DataObject *)malloc(sizeof(DataObject));
    object->home = malloc(strlen(g_get_user_data_dir()) + 20);
    object->store = g_list_store_new(CANDLE_TYPE_OBJECT);
    object->rectangle = (GdkRectangle *)malloc(sizeof(GdkRectangle));
    object->price = (CandlePrice *)malloc(sizeof(CandlePrice));
    object->data = (CandleData *)malloc(sizeof(CandleData));
    object->time = (CandleTime *)malloc(sizeof(CandleTime));
    object->instrument = NULL;
    sprintf(object->home, "%s/binclab/terminal/", g_get_user_data_dir());
    GtkApplication *app = gtk_application_new("com.binclab.terminal", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", (GCallback)activate, object);
    g_application_run((GApplication *)app, argc, argv);
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