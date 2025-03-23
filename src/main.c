#include "database.h"
#include "session.h"
#include "chart.h"

static GLuint compile_shader(GLenum type, const gchar *name)
{
    gchar *resourcePath = g_strdup_printf("/com/binclab/trader/gl/%s", name);
    GBytes *bytes = g_resources_lookup_data(resourcePath, 0, NULL);
    g_free(resourcePath);
    if (!bytes)
    {
        g_warning("ERROR::SHADER::RESOURCE_NOT_FOUND\n");
        return 0;
    }

    gsize size;
    const gchar *shaderCode = (const gchar *)g_bytes_get_data(bytes, &size);
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

static void realize_cartesian(GtkGLArea *area, gpointer userdata)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return;
    }

    GLuint vertex = compile_shader(GL_VERTEX_SHADER, "shader.vert");
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, "shader.frag");

    if (!vertex || !fragment)
    {
        g_warning("Failed to compile shaders");
        return;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *buffer = (GLchar *)g_malloc0(length);
        glGetProgramInfoLog(program, length, NULL, buffer);
        fprintf(stderr, "Program linking failed: %s\n", buffer);
        g_clear_pointer(&buffer, g_free);
        return;
    }

    /*GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);*/

    GObject *line = g_object_new(G_TYPE_OBJECT, NULL);
    gfloat *ordinate = g_new0(gfloat, 1);
    gfloat *red = g_new0(gfloat, 1);
    gfloat *green = g_new0(gfloat, 1);
    gfloat *blue = g_new0(gfloat, 1);
    *ordinate = 0.0f;
    *red = 0.0f;
    *green = 0.0f;
    *blue = 1.0f;
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);
    g_list_store_append(store, line);

    GLuint buffer;
    glGenBuffers(1, &buffer);
    GObject *object = G_OBJECT(area);
    GObject *data = G_OBJECT(g_object_get_data(G_OBJECT(userdata), "data"));
    g_object_set_data(object, "buffer", GUINT_TO_POINTER(buffer));
    g_object_set_data(object, "store", store);
    g_object_set_data(data, "program", GUINT_TO_POINTER(program));
    g_object_set_data(line, "ordinate", ordinate);
    g_object_set_data(line, "red", red);
    g_object_set_data(line, "green", green);
    g_object_set_data(line, "blue", blue);
}

static gboolean render_cartesian(GtkGLArea *area, GdkGLContext *context, gpointer userdata)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return FALSE;
    }

    GObject *candle = G_OBJECT(g_object_get_data(G_OBJECT(userdata), "price"));

    if (!G_IS_OBJECT(candle))
        return FALSE;

    GObject *object = G_OBJECT(area);
    GObject *data = G_OBJECT(g_object_get_data(candle, "data"));
    GObject *stat = G_OBJECT(g_object_get_data(candle, "stat"));
    GListModel *model = G_LIST_MODEL(g_object_get_data(object, "store"));
    GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(object, "buffer"));
    gboolean firstrun = GPOINTER_TO_INT(g_object_get_data(object, "firstrun")) == 0;
    gfloat scale = *(gfloat *)g_object_get_data(stat, "scale");
    gfloat baseline = *(gfloat *)g_object_get_data(stat, "baseline");
    gfloat close = *(gfloat *)g_object_get_data(candle, "close");
    GLuint program = GPOINTER_TO_UINT(g_object_get_data(data, "program"));

    for (gint index = 0; index < g_list_model_get_n_items(model); index++)
    {
        GObject *line = G_OBJECT(g_list_model_get_item(model, index));
        gfloat ordinate = *(gfloat *)g_object_get_data(line, "ordinate");
        gfloat red = *(gfloat *)g_object_get_data(line, "red");
        gfloat green = *(gfloat *)g_object_get_data(line, "green");
        gfloat blue = *(gfloat *)g_object_get_data(line, "blue");
        if (firstrun)
        {
            g_object_set_data((object), "firstrun", GINT_TO_POINTER(firstrun));
        }
        else if (index == 0)
        {
            ordinate = scale * (close - baseline);
        }

        GLfloat vertices[] = {
            -1.0f,
            ordinate,
            red,
            green,
            blue,
            1.0f,
            ordinate,
            red,
            green,
            blue};

        glClear(GL_COLOR_BUFFER_BIT);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glUseProgram(program);

        GLint pos_attr = glGetAttribLocation(program, "position");
        GLint col_attr = glGetAttribLocation(program, "color");

        glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(gfloat), (void *)0);
        glEnableVertexAttribArray(pos_attr);

        glVertexAttribPointer(col_attr, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(gfloat), (void *)(2 * sizeof(gfloat)));
        glEnableVertexAttribArray(col_attr);

        glDrawArrays(GL_LINES, 0, 2);
    }

    return TRUE;
}

void create_chart(GTask *task, gpointer source, gpointer userdata, GCancellable *unused)
{
    GObject *object = G_OBJECT(source);
    GListModel *model = G_LIST_MODEL(g_object_get_data(object, "candles"));
    gint count = g_list_model_get_n_items(model);

    for (gint position = count - 1440; position < count; position++)
    {
        add_candle(object, G_OBJECT(g_list_model_get_item(model, position)));
    }
}

static void scroll_horizontal(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void scroll_vertical(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void setup_header(GObject *object, GtkBox *parent)
{
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_hexpand(button, TRUE);
    gtk_widget_set_size_request(header, -1, 48);
    gtk_box_append(GTK_BOX(header), button);
    gtk_box_append(parent, header);
}

static void setup_content(GObject *object, GtkBox *parent)
{
    GtkAdjustment *chartvadjustment, *charthadjustment, *scaleadjustment, *timeadjustment;
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *navigation = gtk_notebook_new();
    GtkWidget *widget = gtk_overlay_new();
    GtkBox *box = GTK_BOX(container);
    gtk_box_append(box, navigation);
    gtk_box_append(box, widget);
    GtkWidget *shortcuts = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *chartscroll = gtk_scrolled_window_new();
    GtkWidget *scalescroll = gtk_scrolled_window_new();
    GtkWidget *timescroll = gtk_scrolled_window_new();
    GtkWidget *chartport = gtk_viewport_new(NULL, NULL);
    GtkWidget *scaleport = gtk_viewport_new(NULL, NULL);
    GtkWidget *timeport = gtk_viewport_new(NULL, NULL);
    GtkWidget *scaleinfo = gtk_label_new(NULL);

    GtkOverlay *overlay = GTK_OVERLAY(widget);
    gtk_widget_set_hexpand(widget, TRUE);
    // gtk_widget_set_hexpand(chartscroll, TRUE);
    gtk_widget_set_margin_end(chartscroll, 100);
    gtk_widget_set_margin_bottom(chartscroll, 48);
    gtk_overlay_set_child(overlay, chartscroll);
    gtk_widget_set_name(chartport, "chartport");
    g_object_set_data(object, "chartport", chartport);
    GtkScrolledWindow *window = GTK_SCROLLED_WINDOW(chartscroll);
    gtk_scrolled_window_set_child(window, chartport);
    gtk_scrolled_window_set_policy(window, GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
    charthadjustment = gtk_scrolled_window_get_hadjustment(window);
    chartvadjustment = gtk_scrolled_window_get_vadjustment(window);
    g_object_set_data(object, "charthadjustment", charthadjustment);
    g_object_set_data(object, "chartvadjustment", chartvadjustment);

    widget = gtk_fixed_new();
    GtkViewport *viewport = GTK_VIEWPORT(chartport);
    GObject *store = G_OBJECT(g_list_store_new(GTK_TYPE_WIDGET));
    GtkFixed *fixed = GTK_FIXED(widget);
    gtk_widget_set_overflow(widget, GTK_OVERFLOW_VISIBLE);
    gtk_viewport_set_child(viewport, widget);
    g_object_set_data(object, "chartfixed", widget);
    g_object_set_data(store, "position", GINT_TO_POINTER(1000));
    g_object_set_data(G_OBJECT(widget), "store", store);
    widget = gtk_gl_area_new();
    gtk_widget_set_size_request(widget, 1024, 576);
    g_object_set_data(object, "cartesian", widget);
    g_signal_connect(widget, "realize", G_CALLBACK(realize_cartesian), object);
    g_signal_connect(widget, "unrealize", G_CALLBACK(unrealize_cartesian), object);
    g_signal_connect(widget, "render", G_CALLBACK(render_cartesian), object);
    gtk_fixed_put(fixed, widget, 0, 0);

    gtk_overlay_add_overlay(overlay, shortcuts);
    gtk_widget_set_valign(shortcuts, GTK_ALIGN_START);

    gtk_overlay_add_overlay(overlay, timescroll);
    gtk_widget_set_valign(timescroll, GTK_ALIGN_END);
    gtk_widget_set_margin_end(timescroll, 100);
    gtk_widget_set_size_request(timescroll, -1, 48);
    window = GTK_SCROLLED_WINDOW(timescroll);
    gtk_scrolled_window_set_child(window, timeport);
    gtk_scrolled_window_set_policy(window, GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    timeadjustment = gtk_scrolled_window_get_hadjustment(window);
    g_object_set_data(object, "timeadjustment", timeadjustment);

    // box =  GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    // widget = GTK_WIDGET(g_object_new(GTK_TYPE_WIDGET, NULL));
    // gtk_box_append(box, widget);
    widget = gtk_fixed_new();
    viewport = GTK_VIEWPORT(timeport);
    gtk_viewport_set_child(viewport, widget);
    g_object_set_data(object, "timefixed", widget);
    g_object_set_data(G_OBJECT(widget), "store", g_list_store_new(GTK_TYPE_WIDGET));
    gtk_widget_set_name(timeport, "timeport");

    gtk_overlay_add_overlay(overlay, scalescroll);
    gtk_widget_set_halign(scalescroll, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(scalescroll, 48);
    gtk_widget_set_size_request(scalescroll, 100, -1);
    window = GTK_SCROLLED_WINDOW(scalescroll);
    gtk_scrolled_window_set_child(window, scaleport);
    gtk_scrolled_window_set_policy(window, GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
    scaleadjustment = gtk_scrolled_window_get_vadjustment(window);
    g_object_set_data(object, "scaleadjustment", scaleadjustment);

    widget = gtk_fixed_new();
    viewport = GTK_VIEWPORT(scaleport);
    gtk_viewport_set_child(viewport, widget);
    g_object_set_data(object, "scalefixed", widget);

    gtk_overlay_add_overlay(overlay, scaleinfo);
    gtk_widget_set_halign(scaleinfo, GTK_ALIGN_END);
    gtk_widget_set_valign(scaleinfo, GTK_ALIGN_END);
    gtk_widget_set_size_request(scaleinfo, 100, 48);

    gtk_widget_set_vexpand(container, TRUE);
    gtk_widget_set_size_request(navigation, 250, -1);
    gtk_box_append(parent, container);

    g_signal_connect(charthadjustment, "value-changed", G_CALLBACK(scroll_horizontal), timeadjustment);
    g_signal_connect(timeadjustment, "value-changed", G_CALLBACK(scroll_horizontal), charthadjustment);
    g_signal_connect(chartvadjustment, "value-changed", G_CALLBACK(scroll_vertical), scaleadjustment);
    g_signal_connect(scaleadjustment, "value-changed", G_CALLBACK(scroll_vertical), chartvadjustment);
}

static void setup_accounts(GObject *object, GtkBox *parent)
{
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *child = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *label = gtk_label_new("Account Information");
    GtkWidget *accounts = gtk_notebook_new();
    GtkWidget *progress = gtk_progress_bar_new();
    GtkBox *box = GTK_BOX(container);
    gtk_widget_set_size_request(container, 250, 200);
    gtk_widget_set_size_request(label, -1, 32);
    g_object_set_data(object, "progress", progress);
    gtk_box_append(box, label);
    gtk_box_append(box, progress);
    gtk_widget_set_valign(label, GTK_ALIGN_END);
    gtk_widget_set_valign(progress, GTK_ALIGN_START);
    box = GTK_BOX(child);
    gtk_box_append(box, container);
    gtk_box_append(box, accounts);
    gtk_box_append(parent, child);
}

void present_actual_child(GObject *object)
{
    GtkWindow *window = GTK_WINDOW(g_object_get_data(object, "window"));
    GtkWidget *widget = GTK_WIDGET(window);
    GtkWidget *child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkNative *native = gtk_widget_get_native(widget);
    GdkSurface *surface = gtk_native_get_surface(native);
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_monitor_at_surface(display, surface);

    GdkRectangle rectangle;
    gdk_monitor_get_geometry(monitor, &rectangle);
    int height = rectangle.height - 128, width = rectangle.width - 128;
    height = height < 576 ? 576 : height;
    width = width < 1024 ? 1024 : width;
    gtk_widget_set_size_request(widget, width, height);
    gtk_window_set_default_size(window, width, height);
    GtkBox *parent = GTK_BOX(child);
    setup_header(object, parent);
    setup_content(object, parent);
    setup_accounts(object, parent);
    gtk_window_set_child(window, child);
    // g_task_run_in_thread(bincdata->task, setup_soup_session);
}

void setup_webview(GObject *object)
{
    WebKitWebView *webview = WEBKIT_WEB_VIEW(g_object_get_data(object, "webview"));
    WebKitNetworkSession *session = webkit_web_view_get_network_session(webview);
    WebKitCookieManager *cookiejar = webkit_network_session_get_cookie_manager(session);
    const gchar *home = gtk_string_object_get_string(GTK_STRING_OBJECT(object));
    gsize maxlength = strlen(home) + 11;
    gchar *storage = (gchar *)g_malloc0(maxlength);
    g_snprintf(storage, maxlength, "%sstorage.db", home);
    gchar *uri = "https://oauth.deriv.com/oauth2/authorize?app_id=66477";
    webkit_cookie_manager_set_persistent_storage(cookiejar, storage, 1);
    webkit_web_view_load_uri(webview, uri);
    g_signal_connect(webview, "load-changed", G_CALLBACK(load_changed), object);
    g_clear_pointer(&storage, g_free);
    GtkWindow *window = GTK_WINDOW(g_object_get_data(object, "window"));
    gtk_window_set_child(window, GTK_WIDGET(webview));
}

static void activate(GtkApplication *application, gpointer userdata)
{
    GObject *object = G_OBJECT(userdata);
    GtkWindow *window = GTK_WINDOW(gtk_application_window_new(application));
    g_object_set_data(object, "window", window);
    g_object_set_data(object, "webview", webkit_web_view_new());
    const char *styles = "/com/binclab/trader/css/style.css";
    GdkDisplay *display = gdk_display_get_default();
    GtkStyleProvider *provider = GTK_STYLE_PROVIDER(gtk_css_provider_new());
    GtkIconTheme *theme = gtk_icon_theme_get_for_display(display);

    gtk_widget_set_size_request(GTK_WIDGET(window), 1024, 576);
    gtk_window_set_title(window, "Binc Trader");
    gtk_window_set_default_size(window, 1024, 576);
    g_signal_connect(window, "close-request", G_CALLBACK(close_window), object);
    gtk_icon_theme_add_resource_path(theme, "/com/binclab/trader/icons/96x96");
    gtk_css_provider_load_from_resource((GtkCssProvider *)provider, styles);
    gtk_style_context_add_provider_for_display(display, provider, 600);

    GListStore *profile = g_list_store_new(GTK_TYPE_STRING_OBJECT);
    GListStore *candles = g_list_store_new(G_TYPE_OBJECT);
    GObject *stat = g_object_new(G_TYPE_OBJECT, NULL);
    GObject *data = g_object_new(G_TYPE_OBJECT, NULL);

    gfloat *highest = g_new0(gfloat, 1);
    gfloat *lowest = g_new0(gfloat, 1);
    gfloat *range = g_new0(gfloat, 1);
    gfloat *baseline = g_new0(gfloat, 1);
    gfloat *factor = g_new0(gfloat, 1);

    g_object_set_data(stat, "highest", highest);
    g_object_set_data(stat, "lowest", lowest);
    g_object_set_data(stat, "range", range);
    g_object_set_data(stat, "baseline", baseline);
    g_object_set_data(stat, "factor", factor);
    g_object_set_data(object, "stat", stat);
    g_object_set_data(object, "data", data);
    g_object_set_data(object, "candles", candles);
    g_object_set_data(object, "profile", profile);

    setup_user_interface(object);
}

static void handle_prepare_for_sleep(
    GDBusConnection *connection,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer userdata)
{
    gboolean going_to_sleep;
    g_variant_get(parameters, "(b)", &going_to_sleep);

    if (going_to_sleep)
    {
        g_print("System is suspending, save state...\n");
        // Save your application state here
    }
    else
    {
        g_print("System is resuming, restore state...\n");
        // Restore your application state here
    }
}

gint main(int argc, char *argv[])
{
    gsize maxlength = strlen(g_get_user_data_dir()) + 13;
    gchar *buffer = g_malloc0(maxlength);
    g_snprintf(buffer, maxlength, "%s/binctrader/", g_get_user_data_dir());
    GtkStringObject *object = gtk_string_object_new(buffer);
    g_clear_pointer(&buffer, g_free);
    GtkApplication *app = gtk_application_new("com.binclab.trader", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), object);
    g_signal_connect(app, "shutdown", G_CALLBACK(shutdown), object);
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    if (connection)
    {
        g_dbus_connection_signal_subscribe(
            connection,
            "org.freedesktop.login1",
            "org.freedesktop.login1.Manager",
            "PrepareForSleep",
            "/org/freedesktop/login1",
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            handle_prepare_for_sleep,
            NULL,
            NULL);
    }
    gint status = g_application_run(G_APPLICATION(app), argc, argv);
    // g_object_unref(task);
    g_object_unref(app);
    // if (connection)
    //{
    // g_object_unref(connection);
    //}
    return status;
}