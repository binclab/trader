#include "main.h"

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

    if (close_value == upper)
    {
        double mark = upper + window->model->data->space;
        gtk_adjustment_set_upper(adjustment, mark);
        snprintf(buffer, 10, "%.3f", mark);
        gtk_scale_add_mark((GtkScale *)scale, mark, GTK_POS_BOTTOM, buffer);
    }
    else if (close_value == lower)
    {
        double mark = lower - window->model->data->space;
        gtk_adjustment_set_lower(adjustment, mark);
        snprintf(buffer, 10, "%.3f", mark);
        gtk_scale_add_mark((GtkScale *)scale, mark, GTK_POS_BOTTOM, buffer);
    }
}

static void setup_window(BincWindow *window, gchar *home)
{

    GtkAdjustment *vertical = gtk_adjustment_new(window->height, 0, window->height, 0.01, 0.1, 10);
    GtkAdjustment *horizontal = gtk_adjustment_new(window->width, 0, window->width, 48, 48, 48);
    layout1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    layout2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    canvasbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    scalebox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    layout4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    scaleinfo = gtk_label_new(NULL);
    chart = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    timechart = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    header1 = gtk_button_new();
    infoarea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    infobox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    termspace = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    termframe = gtk_frame_new("Infomation");
    terminfo = gtk_label_new("No bytes used");
    termbar = gtk_action_bar_new();
    conled = gtk_toggle_button_new();
    termbar = gtk_action_bar_new();
    accountbook = gtk_notebook_new();
    navigation = gtk_button_new();
    scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.0, window->height, 0.001);
    chartport = gtk_viewport_new(horizontal, vertical);
    timeport = gtk_viewport_new(horizontal, NULL);
    scaleport = gtk_viewport_new(NULL, vertical);
    chartscroll = gtk_scrolled_window_new();
    timescroll = gtk_scrolled_window_new();
    scalescroll = gtk_scrolled_window_new();

    window->scrollinfo = gtk_scroll_info_new();
    window->timeport = (GtkViewport *)timeport;
    window->scale = (GtkRange *)scale;
    window->infolabel = (GtkLabel *)terminfo;
    window->led = (GtkToggleButton *)conled;

    window->model->timeframe = "1 MINUTE";
    window->model->home = window->home;
    window->model->data->box = (GtkBox *)timechart;
}

static void setup_children(BincWindow *window)
{
    gtk_widget_set_size_request(navigation, 200, -1);
    gtk_widget_set_size_request(header1, -1, 48);
    gtk_widget_set_size_request(timeport, -1, 48);
    gtk_widget_set_size_request(scaleinfo, -1, 48);
    gtk_widget_set_size_request(termspace, -1, -1);
    gtk_widget_set_size_request(termframe, 200, 200);
    gtk_widget_set_size_request(scalebox, 108, window->height - 48);
    gtk_widget_set_size_request(scalescroll, 108, window->height - 48);
    gtk_widget_set_size_request(canvasbox, window->width, window->height);
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
    gtk_widget_add_css_class(conled, "circular");

    gtk_label_set_justify((GtkLabel *)scaleinfo, GTK_JUSTIFY_CENTER);
    gtk_notebook_set_tab_pos((GtkNotebook *)accountbook, GTK_POS_LEFT);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)chartscroll, GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
    gtk_scrolled_window_set_policy((GtkScrolledWindow *)timescroll, GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
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
    gtk_viewport_set_child((GtkViewport *)chartport, chart);
    gtk_viewport_set_child((GtkViewport *)timeport, timechart);
    gtk_viewport_set_child((GtkViewport *)scaleport, scale);

    gtk_scrolled_window_set_child((GtkScrolledWindow *)chartscroll, chartport);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)timescroll, timeport);
    gtk_scrolled_window_set_child((GtkScrolledWindow *)scalescroll, scaleport);

    gtk_action_bar_pack_start((GtkActionBar *)termbar, conled);
    gtk_box_append((GtkBox *)infobox, terminfo);
    gtk_box_append((GtkBox *)infobox, termspace);
    gtk_box_append((GtkBox *)infobox, termbar);
    gtk_frame_set_child((GtkFrame *)termframe, infobox);

    // gtk_box_append((GtkBox *)scalebox, scalescroll);
    gtk_box_append((GtkBox *)infoarea, termframe);
    gtk_box_append((GtkBox *)infoarea, accountbook);
    gtk_box_append((GtkBox *)layout4, scalescroll);
    gtk_box_append((GtkBox *)layout4, scaleinfo);
    gtk_box_append((GtkBox *)canvasbox, chartscroll);
    gtk_box_append((GtkBox *)canvasbox, timescroll);
    gtk_box_append((GtkBox *)layout2, navigation);
    gtk_box_append((GtkBox *)layout2, canvasbox);
    gtk_box_append((GtkBox *)layout2, layout4);
    gtk_box_append((GtkBox *)layout1, header1);
    gtk_box_append((GtkBox *)layout1, layout2);
    gtk_box_append((GtkBox *)layout1, infoarea);
}

void create_chart(BincWindow *window, guint last)
{
    Candle **candles = window->candleinfo->candles;

    for (int position = 0; position < last; position++)
    {
        
        candles[position]->data = window->model->data;
        candles[position]->time = window->model->time;
        // if (position == 0 && same)
        //     gtk_widget_queue_draw(window->widget);
        // else
        gtk_box_append((GtkBox *)chart, create_canvas(candles[position]));
        //free(candles[position]);
    }
    window->widget = create_canvas(candles[last]);
    gtk_box_append((GtkBox *)chart, window->widget);
    GtkAdjustment *adjustment = gtk_range_get_adjustment((GtkRange *)scale);
    gtk_adjustment_set_upper(adjustment, window->candle->data->highest);
    gtk_adjustment_set_lower(adjustment, window->candle->data->lowest);
    window->candle->data->space = (window->candle->data->highest - window->candle->data->lowest) / 20;
    window->candle->data->height = window->height / 20;
    for (double index = window->candle->data->lowest; index < window->candle->data->highest; index += window->candle->data->space)
    {
        char buffer[10];
        snprintf(buffer, 9, "%.3f", index);
        gtk_scale_add_mark((GtkScale *)scale, index, GTK_POS_BOTTOM, buffer);
    }
    g_signal_connect(scale, "value-changed", (GCallback)update_scale, window);
}

static GLuint compile_shader(GLenum type, const char *name)
{
    char *resourcePath = malloc(strlen(name) + 26);
    sprintf(resourcePath, "/com/binclab/terminal/gl/%s", name);
    GBytes *bytes = g_resources_lookup_data(resourcePath, 0, NULL);
    free(resourcePath);
    if (!bytes)
    {
        g_warning("ERROR::SHADER::RESOURCE_NOT_FOUND\n");
        return 0;
    }

    gsize size;
    const char *shaderCode = (const gchar *)g_bytes_get_data(bytes, &size);
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);
    g_bytes_unref(bytes);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        g_warning("ERROR::SHADER::COMPILATION_FAILED\n%s", infoLog);
        return 0;
    }
    return shader;
}

static void notify_window_visible(GtkWindow *window, GParamSpec *pspec, gchar *home)
{
    ((BincWindow *)window)->home = home;
    ((BincWindow *)window)->candle->data->vertex = compile_shader(GL_VERTEX_SHADER, "shader.vert");
    ((BincWindow *)window)->candle->data->fragment = compile_shader(GL_FRAGMENT_SHADER, "shader.frag");
    pthread_create(&((BincWindow *)window)->thread, NULL, setup_soup_session, (void *)window);
    setup_window((BincWindow *)window, home);
    setup_children((BincWindow *)window);
    setup_signals((BincWindow *)window);
    add_children();
    gtk_window_set_child(window, layout1);
    g_signal_handlers_disconnect_by_func(window, notify_window_visible, home);
}

static void activate_application(GtkApplication *app, gchar *home)
{
    GtkWidget *window = binc_window_new(app);
    GtkNative *native = gtk_widget_get_native(window);
    create_symbol_database(home);
    gtk_window_set_title((GtkWindow *)window, "Binc Terminal");
    const char *styles = "/com/binclab/terminal/css/style.css";
    display = gdk_display_get_default();
    networkmonitor = g_network_monitor_get_default();
    provider = (GtkStyleProvider *)gtk_css_provider_new();
    theme = gtk_icon_theme_get_for_display(display);
    surface = gtk_native_get_surface(native);
    gtk_icon_theme_add_resource_path(theme, "/com/binclab/terminal/icons/96x96");
    gtk_css_provider_load_from_resource((GtkCssProvider *)provider, styles);
    gtk_style_context_add_provider_for_display(display, provider, 600);
    // GdkMonitor *monitor = g_list_model_get_item(gdk_display_get_monitors(display), 0);
    gtk_window_present((GtkWindow *)window);
    g_signal_connect(window, "notify::focus-visible", (GCallback)notify_window_visible, home);
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
    gchar *home = malloc(strlen(g_get_user_data_dir()) + 20);
    sprintf(home, "%s/binclab/terminal/", g_get_user_data_dir());
    GtkApplication *app = gtk_application_new("com.binclab.terminal", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", (GCallback)activate_application, home);
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