/*
 * Copyright (C) 2024  Bret Joseph Antonio
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <webkit/webkit.h>

static void prevent_zoom(GtkGestureZoom *controller, double scale, gpointer user_data)
{
    gtk_event_controller_reset((GtkEventController*)controller);
    printf("Zooming\n");
}

static void setup_webview(WebKitWebView *webview, WebKitLoadEvent load_event, gpointer user_data)
{
    const char *script = "window.addEventListener('wheel', (e) => {"
                         "    if(e.ctrlKey) e.preventDefault();"
                         "}, { passive: false });";
    switch (load_event)
    {
    case WEBKIT_LOAD_STARTED:
        break;
    case WEBKIT_LOAD_REDIRECTED:
        break;
    case WEBKIT_LOAD_COMMITTED:
        break;
    case WEBKIT_LOAD_FINISHED:
        webkit_web_view_evaluate_javascript(webview, script, -1, NULL, NULL, NULL, NULL, NULL);
        break;
    }
}

static void create_window(GtkApplication *app, gpointer user_data)
{
    const gchar *uri = "https://mt5-real02-web-svg.deriv.com/terminal?login=101302275&server=DerivSVG-Server-02";
    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *webview = webkit_web_view_new();
    gtk_window_set_child((GtkWindow *)window, webview);
    gtk_window_present((GtkWindow *)window);
    gtk_window_set_title((GtkWindow *)window, "MT5 Terminal");
    gtk_widget_set_size_request(window, 1024, 576);
    gtk_window_set_default_size((GtkWindow *)window, 1024, 576);
    // WebKitSettings *settings = webkit_web_view_get_settings((WebKitWebView *)webview);
    WebKitNetworkSession *session = webkit_web_view_get_network_session((WebKitWebView *)webview);
    WebKitCookieManager *cookiejar = webkit_network_session_get_cookie_manager(session);
    webkit_cookie_manager_set_persistent_storage(cookiejar, "storage", WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    webkit_web_view_load_uri((WebKitWebView *)webview, uri);
    // g_signal_connect(webview, "load-changed", (GCallback)setup_webview, NULL);
    GtkGesture *controller = gtk_gesture_zoom_new();
    g_signal_connect(controller, "scale-changed", (GCallback)prevent_zoom, NULL);
    gtk_widget_add_controller(webview, (GtkEventController*)controller);
}

int main(int response, char **name)
{
    printf("Starting!...\n");
    GtkApplication *app = gtk_application_new("com.binclab.mt5terminal", 0);
    g_signal_connect(app, "activate", (GCallback)create_window, NULL);
    response = g_application_run((GApplication *)app, response, name);
    g_object_unref(app);
    printf("Goodbye!...\n");
    return response;
}
