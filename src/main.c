#include "cleanup.h"
#include "database.h"
#include "session.h"

void present_actual_child(GObject *task)
{
    GtkWindow *window = GTK_WINDOW(g_object_get_data(task, "window"));
    GtkWidget *widget = GTK_WIDGET(window);
    GtkWidget *child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkNative *native = gtk_widget_get_native(widget);
    GdkSurface *surface = gtk_native_get_surface(native);
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_monitor_at_surface(display, surface);

    GdkRectangle rectangle;
    gdk_monitor_get_geometry(monitor, &rectangle);
    int height = rectangle.height - 128, width = rectangle.width - 128;
    height = height < 768 ? 768 : height;
    width = width < 1024 ? 1024 : width;
    gtk_widget_set_size_request(widget, width, height);
    gtk_window_set_default_size(window, width, height);
    gtk_window_set_child(window, child);
    // g_task_run_in_thread(bincdata->task, setup_soup_session);
}

void setup_webview(GObject *task)
{
    WebKitWebView *webview = WEBKIT_WEB_VIEW(g_object_get_data(task, "webview"));
    WebKitNetworkSession *session = webkit_web_view_get_network_session(webview);
    WebKitCookieManager *cookiejar = webkit_network_session_get_cookie_manager(session);
    const gchar *home = g_object_get_data(task, "home");
    gsize maxlength = strlen(home) + 11;
    gchar *storage = (gchar *)g_malloc0(maxlength);
    g_snprintf(storage, maxlength, "%sstorage.db", home);
    gchar *uri = "https://oauth.deriv.com/oauth2/authorize?app_id=66477";
    webkit_cookie_manager_set_persistent_storage(cookiejar, storage, 1);
    webkit_web_view_load_uri(webview, uri);
    g_signal_connect(webview, "load-changed", G_CALLBACK(load_changed), task);
    g_clear_pointer(&storage, g_free);
    GtkWindow *window = GTK_WINDOW(g_object_get_data(task, "window"));
    gtk_window_set_child(window, GTK_WIDGET(webview));
}

static void activate(GtkApplication *application, gpointer userdata)
{
    GObject *task = G_OBJECT(userdata);
    GtkWindow *window = GTK_WINDOW(gtk_application_window_new(application));
    g_object_set_data(task, "window", window);
    g_object_set_data(task, "webview", webkit_web_view_new());
    const char *styles = "/com/binclab/trader/css/style.css";
    GdkDisplay *display = gdk_display_get_default();
    GtkStyleProvider *provider = GTK_STYLE_PROVIDER(gtk_css_provider_new());
    GtkIconTheme *theme = gtk_icon_theme_get_for_display(display);

    gtk_widget_set_size_request(GTK_WIDGET(window), 1024, 768);
    gtk_window_set_title(window, "Binc Trader");
    gtk_window_set_default_size(window, 1024, 768);
    gtk_icon_theme_add_resource_path(theme, "/com/binclab/trader/icons/96x96");
    gtk_css_provider_load_from_resource((GtkCssProvider *)provider, styles);
    gtk_style_context_add_provider_for_display(display, provider, 600);

    GListStore *profile = g_list_store_new(GTK_TYPE_STRING_OBJECT);
    GListStore *candles = g_list_store_new(G_TYPE_OBJECT);
    gdouble *stat = g_new(gdouble, 5);
    g_object_set_data(task, "stat", stat);
    g_object_set_data(task, "candles", candles);
    g_object_set_data(task, "profile", profile);

    setup_user_interface(task);
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
    GTask *task = g_task_new(NULL, NULL, NULL, NULL);
    gsize maxlength = strlen(g_get_user_data_dir()) + 13;
    gchar *home = g_malloc0(maxlength);
    g_snprintf(home, maxlength, "%s/binctrader/", g_get_user_data_dir());
    g_object_set_data(G_OBJECT(task), "home", home);
    GtkApplication *app = gtk_application_new("com.binclab.trader", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), task);
    g_signal_connect(app, "shutdown", G_CALLBACK(shutdown), task);
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
    g_object_unref(task);
    g_object_unref(app);
    if (connection)
    {
        g_object_unref(connection);
    }
    return status;
}