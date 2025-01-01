#include "interval.h"
#include <gio/gio.h>
/*
G_DEFINE_TYPE(BincInterval, binc_interval, GTK_TYPE_GL_AREA)

static CandleInfo *info;

static GdkGLContext *create_context(GtkGLArea *area, GError *error)
{
    GtkNative *native = gtk_widget_get_native((GtkWidget *)area);
    GdkSurface *surface = gtk_native_get_surface(native);
    GdkGLContext *context = gdk_surface_create_gl_context(surface, &error);
    gdk_gl_context_set_debug_enabled(context, TRUE);
    gdk_gl_context_set_required_version(context, 3, 0);
    return context;
}

static void realize_interval(GtkGLArea *area)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return;
    }

    if (gtk_gl_area_get_api(area) != GDK_GL_API_GLES)
    {
        g_warning("Unsupported GL API. Only GLES is supported.");
        return;
    }

    GLuint vao;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &info->buffer);
}

static float getCoord(double size, double port)
{
    return 2 * size / port;
}

static gboolean render_interval(GtkGLArea *area, GdkGLContext *context)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
        return FALSE;

    float vertices[108] = {

        // Triangle Line 1 TOP
        getCoord(-1, 64),
        getCoord(info->candle->high - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        getCoord(1, 64),
        getCoord(info->candle->high - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        getCoord(-1, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        // Triangle Line 1 BOTTOM
        getCoord(1, 64),
        getCoord(info->candle->high - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        getCoord(-1, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        getCoord(1, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        // Triangle Line 2 TOP
        getCoord(1, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        getCoord(1, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        getCoord(-1, 64),
        getCoord(info->candle->low - info->lowest, info->height),
        1.0f,
        0.0f,
        0.0f,
        1.0f,

        // Triangle Line 2 BOTTOM
        getCoord(-1, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        1.0f,
        0.0f,
        0.0f,
        1.0f,

        getCoord(-1, 64),
        getCoord(info->candle->low - info->lowest, info->height),
        1.0f,
        0.0f,
        0.0f,
        1.0f,

        getCoord(1, 64),
        getCoord(info->candle->low - info->lowest, info->height),
        1.0f,
        0.0f,
        0.0f,
        1.0f,

        // Triangle Line 3 TOP
        getCoord(-4, 64),
        getCoord(info->candle->close - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        getCoord(4, 64),
        getCoord(info->candle->close - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        getCoord(-4, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        // Triangle Line 3 BOTTOM
        getCoord(4, 64),
        getCoord(info->candle->close - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        getCoord(-4, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        getCoord(4, 64),
        getCoord(info->candle->open - info->lowest, info->height),
        0.0f,
        1.0f,
        0.0f,
        1.0f,
    };

    if (info->candle->open > info->candle->close)
    {
        vertices[75] = vertices[81] = vertices[87] = vertices[93] = vertices[99] = vertices[105] = 1.0f;
        vertices[76] = vertices[82] = vertices[88] = vertices[94] = vertices[100] = vertices[106] = 0.0f;
    }

    GLfloat VERTEX_DATA[] = {
        0.0f, 0.5f,
        0.5f, -0.5f,
        -0.5f, -0.5f};
    glBindBuffer(GL_ARRAY_BUFFER, info->buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_DATA), VERTEX_DATA, GL_STATIC_DRAW);
    glUseProgram(info->program);

    GLint pos_attr = glGetAttribLocation(info->program, "position");
    glEnableVertexAttribArray(pos_attr);
    glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    // Clear the screen and draw the triangle
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    return TRUE;
}

static void binc_interval_class_init(BincIntervalClass *class) {}

static void binc_interval_init(BincInterval *self)
{
    gint hours = g_date_time_get_hour(info->candle->epoch);
    gint minutes = g_date_time_get_minute(info->candle->epoch);
    gchar *time = g_strdup_printf("%02i:%02i", hours, minutes);

    GtkWidget *label = gtk_label_new(time);
    g_free(time);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_vexpand(GTK_WIDGET(self), TRUE);
    gtk_widget_set_size_request(label, 64, 48);
    gtk_widget_set_size_request(GTK_WIDGET(self), 64, -1);
    gtk_box_append(GTK_BOX(info->box), label);

    gtk_gl_area_set_required_version((GtkGLArea *)self, 3, 0);
    gtk_gl_area_set_has_depth_buffer((GtkGLArea *)self, TRUE);
    gtk_gl_area_set_has_stencil_buffer((GtkGLArea *)self, TRUE);
    gtk_gl_area_set_allowed_apis((GtkGLArea *)self, GDK_GL_API_GLES);
    g_signal_connect(self, "create-context", (GCallback)create_context, NULL);
    g_signal_connect(self, "realize", (GCallback)realize_interval, NULL);
    // g_signal_connect(self, "unrealize", (GCallback)unrealize_interval, NULL);
    g_signal_connect(self, "render", (GCallback)render_interval, NULL);
}

GtkWidget *binc_interval_new(CandleInfo *value)
{
    info = value;
    return g_object_new(BINC_TYPE_INTERVAL, NULL);
}*/