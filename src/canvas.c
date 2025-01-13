#include "symbols.h"

static void bind_vertices(GtkWidget *widget, BincCandle *candle)
{
    GLfloat scale = CANDLE_HEIGHT / candle->stat->scale;
    gboolean bearish = candle->price->open > candle->price->close;
    GLfloat red = bearish ? 1.0f : 0.0f;
    GLfloat green = bearish ? 0.0f : 1.0f;

    /*float high = candle->data->scale * (candle->price->high - candle->data->baseline);
    float open = candle->data->scale * (candle->price->open - candle->data->baseline);
    float close = candle->data->scale * (candle->price->close - candle->data->baseline);
    float low = candle->data->scale * (candle->price->low - candle->data->baseline);*/

    GLfloat high = scale * (candle->price->high - candle->stat->baseline);
    GLfloat low = scale * (candle->price->low - candle->stat->baseline);
    gdouble maximum = fmax(fabs(high), fabs(low));
    gint factor = 1;
    if (maximum > 1.0f)
    {
        factor = 2;
        scale = CANDLE_HEIGHT / (candle->stat->scale * factor);
    }
    else if (maximum > 2.0f)
    {
        factor = 3;
        scale = CANDLE_HEIGHT / (candle->stat->scale * factor);
    }
    high = scale * (candle->price->high - candle->stat->baseline);
    low = scale * (candle->price->low - candle->stat->baseline);
    GLfloat open = scale * (candle->price->open - candle->stat->baseline);
    GLfloat close = scale * (candle->price->close - candle->stat->baseline);
    gtk_widget_set_size_request(widget, CANDLE_WIDTH, CANDLE_HEIGHT * factor);
    //gtk_widget_set_size_request(gtk_widget_get_parent(widget), CANDLE_WIDTH, CANDLE_HEIGHT * factor);

    GLfloat vertices[126] = {
        // Triangle BincLine 1
        -THIN_LINE, open, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        THIN_LINE, high, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -THIN_LINE, high, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        THIN_LINE, high, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -THIN_LINE, open, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        THIN_LINE, open, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

        // Triangle BincLine 2
        -THIN_LINE, open, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        THIN_LINE, open, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -THIN_LINE, low, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        THIN_LINE, low, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -THIN_LINE, low, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        THIN_LINE, open, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        // Triangle BincLine 3
        -THICK_LINE, open, 0.0f, red, green, 0.0f, 1.0f,
        THICK_LINE, close, 0.0f, red, green, 0.0f, 1.0f,
        -THICK_LINE, close, 0.0f, red, green, 0.0f, 1.0f,
        THICK_LINE, close, 0.0f, red, green, 0.0f, 1.0f,
        -THICK_LINE, open, 0.0f, red, green, 0.0f, 1.0f,
        THICK_LINE, open, 0.0f, red, green, 0.0f, 1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

static gboolean render_canvas(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{ // Draw A Candle of Height
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return FALSE;
    }

    BincCandle *candle = BINC_CANDLE(user_data);
    guint *buffer = (guint *)g_object_get_data(G_OBJECT(area), "buffer");

    // glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, *buffer);
    bind_vertices(GTK_WIDGET(area), candle);
    glUseProgram(candle->data->program);

    GLint pos_attr = glGetAttribLocation(candle->data->program, "position");
    GLint col_attr = glGetAttribLocation(candle->data->program, "color");
    glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(pos_attr);
    glVertexAttribPointer(col_attr, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(col_attr);

    glDrawArrays(GL_TRIANGLES, 0, 18);
    glBindVertexArray(0);
    return TRUE;
}

static void realize_canvas(GtkGLArea *area, CandleData *data)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return;
    }

    /*GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);*/

    CandleBuffer *buffer = g_new0(CandleBuffer, 1);
    buffer->vbo = 0;
    g_object_set_data(G_OBJECT(area), "buffer", buffer);
    glGenBuffers(1, &buffer->vbo);
}

static void unrealize_canvas(GtkGLArea *area, BincCandle *candle)
{
    if (gtk_gl_area_get_error(area) == NULL)
    {
        CandleBuffer *buffer = g_object_get_data(G_OBJECT(area), "buffer");
        glDeleteVertexArrays(1, &buffer->vbo);
        g_free(buffer);
        g_date_time_unref(candle->price->epoch);
        free(candle->price);
        g_object_unref(candle);
    }
}

static GdkGLContext *create_context(GtkGLArea *area, GError *error)
{
    GtkNative *native = gtk_widget_get_native((GtkWidget *)area);
    GdkSurface *surface = gtk_native_get_surface(native);
    GdkGLContext *context = gdk_surface_create_gl_context(surface, &error);
    gdk_gl_context_set_allowed_apis(context, GDK_GL_API_GLES);
    gdk_gl_context_set_debug_enabled(context, TRUE);
    gdk_gl_context_set_forward_compatible(context, TRUE);
    gdk_gl_context_set_required_version(context, 3, 0);
    return context;
}

GtkFixed *add_candle(BincData *bincdata, BincCandle *candle)
{
    gint hours = g_date_time_get_hour(candle->price->epoch) + candle->time->hours;
    gint minutes = g_date_time_get_minute(candle->price->epoch) + candle->time->minutes;
    gboolean even = minutes % 5 == 0;
    gchar *name = even ? g_strdup_printf("%02i:%02i", hours, minutes) : NULL;
    GtkWidget *fixed = gtk_fixed_new();
    GtkWidget *line = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *label = gtk_label_new(name);
    bincdata->widget = gtk_gl_area_new();
    g_free(name);

    gtk_fixed_put((GtkFixed *)fixed, line, 1.7, 0);
    gtk_gl_area_set_has_stencil_buffer((GtkGLArea *)bincdata->widget, TRUE);
    gtk_widget_set_vexpand(fixed, TRUE);
    gtk_widget_set_vexpand(line, TRUE);
    gtk_widget_set_halign(line, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(fixed, GTK_ALIGN_CENTER);
    // gtk_widget_set_halign(line, TRUE);
    gtk_widget_set_hexpand(line, FALSE);
    gtk_widget_set_size_request(fixed, CANDLE_WIDTH, CANDLE_HEIGHT);
    gtk_widget_set_size_request(bincdata->widget, CANDLE_WIDTH, CANDLE_HEIGHT);
    gtk_widget_set_size_request(label, CANDLE_WIDTH * 5, 48);
    // gtk_widget_set_margin_start(label, 3);
    gtk_widget_set_margin_start(label, 3);
    gtk_widget_set_size_request(line, CANDLE_WIDTH / 2, CANDLE_HEIGHT);
    // gtk_widget_remove_css_class(glarea, "background");
    gtk_widget_set_valign(bincdata->widget, GTK_ALIGN_CENTER);
    gtk_widget_set_name(bincdata->widget, "candle");
    gtk_widget_set_name(label, "time");
    gtk_widget_set_name(line, "graduation");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    if (even)
        gtk_box_append(bincdata->data->box, label);

    // double value = CANDLE_HEIGHT * 2 / candle->data->range;
    // gdouble ordinate = (candle->data->baseline - candle->price->open) * value;
    // gtk_fixed_put(bincdata->data->overlay, fixed, bincdata->data->abscissa, ordinate);
    // bincdata->data->abscissa += CANDLE_WIDTH;
    gtk_box_append(bincdata->data->chart, fixed);
    //  gtk_widget_realize(bincdata->widget);
    g_signal_connect(bincdata->widget, "create-context", (GCallback)create_context, NULL);
    g_signal_connect(bincdata->widget, "unrealize", (GCallback)unrealize_canvas, candle);
    g_signal_connect(bincdata->widget, "realize", (GCallback)realize_canvas, candle->data);
    g_signal_connect(bincdata->widget, "render", (GCallback)render_canvas, candle);

    double distance = 720 * (candle->stat->highest - candle->price->open) / candle->stat->range;
    gtk_fixed_put((GtkFixed *)fixed, bincdata->widget, 0, 0);
    return (GtkFixed *)fixed;
}