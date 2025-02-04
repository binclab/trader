#include "symbols.h"

static void bind_vertices(GtkWidget *widget, BincCandle *candle)
{

    GtkWidget *parent = gtk_widget_get_parent(widget);
    GdkRectangle rectangle;
    rectangle.height = gtk_widget_get_height(parent);
    rectangle.width = gtk_widget_get_width(parent);
    gboolean bearish = candle->price->open > candle->price->close;
    GLfloat red = bearish ? 1.0f : 0.0f;
    GLfloat green = bearish ? 0.0f : 1.0f;

    gfloat high = candle->stat->scale * (candle->price->high - candle->stat->baseline);
    gfloat low = candle->stat->scale * (candle->price->low - candle->stat->baseline);

    gfloat scale = candle->stat->scale;
    gfloat maximum = fmaxf(fabsf(high), fabsf(low));

    if (maximum > 1.0f)
    {
        gint height = ceilf(CANDLE_HEIGHT * maximum);
        gtk_widget_set_size_request(widget, CANDLE_WIDTH, height);
        scale = scale * CANDLE_HEIGHT / height;
    }
    high = scale * (candle->price->high - candle->stat->baseline);
    low = scale * (candle->price->low - candle->stat->baseline);

    gfloat close = scale * (candle->price->close - candle->stat->baseline);
    gfloat open = scale * (candle->price->open - candle->stat->baseline);

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
    // GLuint buffer;
    // glGenBuffers(1, &buffer);
    //  glBindBuffer(GL_ARRAY_BUFFER, *buffer);
    // g_object_set_data(G_OBJECT(area), "buffer", &buffer);
    GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(area), "buffer"));

    // glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
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

    GLuint buffer;
    glGenBuffers(1, &buffer);
    g_object_set_data(G_OBJECT(area), "buffer", GUINT_TO_POINTER(buffer));
}

static void unrealize_canvas(GtkGLArea *area, BincCandle *candle)
{
    if (gtk_gl_area_get_error(area) == NULL)
    {
        GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(area), "buffer"));
        glDeleteVertexArrays(1, &buffer);
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

void add_candle(BincData *bincdata, BincCandle *candle, gboolean current)
{
    GDateTime *utctime = g_date_time_new_from_unix_utc(candle->price->epoch);
    GDateTime *localtime = g_date_time_to_local(utctime);
    gint hours = g_date_time_get_hour(localtime);     // + candle->time->hours;
    gint minutes = g_date_time_get_minute(localtime); // + candle->time->minutes;
    gboolean even = minutes % 5 == 0;
    gchar *name = even ? g_strdup_printf("%02i:%02i", hours, minutes) : NULL;
    GtkWidget *line = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *label = gtk_label_new(name);
    GtkWidget *widget = gtk_gl_area_new();
    candle->area = GTK_GL_AREA(widget);
    g_free(name);

    if (current)
    {
        GObject *object = G_OBJECT(widget);
        g_object_set_data(object, "area", bincdata->widget);
        bincdata->widget = widget;
        bincdata->price = candle->price;
        g_object_set_data(object, "candle", candle);
    }

    gtk_gl_area_set_has_stencil_buffer(candle->area, TRUE);
    gtk_widget_set_vexpand(line, TRUE);
    gtk_widget_set_vexpand(widget, FALSE);
    gtk_widget_set_halign(line, GTK_ALIGN_CENTER);
    // gtk_widget_set_halign(line, TRUE);
    gtk_widget_set_hexpand(line, FALSE);
    gtk_widget_set_size_request(widget, CANDLE_WIDTH, CANVAS_HEIGHT);
    gtk_widget_set_size_request(label, CANDLE_WIDTH * 5, 48);
    // gtk_widget_set_margin_start(label, 3);
    gtk_widget_set_margin_start(label, 3);
    gtk_widget_set_size_request(line, CANDLE_WIDTH / 2, CANVAS_HEIGHT);
    // gtk_widget_remove_css_class(glarea, "background");
    gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);
    gtk_widget_set_name(widget, "candle");
    gtk_widget_set_name(label, "time");
    gtk_widget_set_name(line, "graduation");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    if (even)
    {
        gtk_box_append(bincdata->data->box, label);
        GListStore *model = G_LIST_STORE(g_object_get_data(G_OBJECT(bincdata->data->box), "model"));
        g_list_store_append(model, label);
    }
    // double value = CANDLE_HEIGHT * 2 / candle->data->range;
    // gdouble ordinate = (candle->data->baseline - candle->price->open) * value;
    // gtk_fixed_put(bincdata->data->overlay, fixed, bincdata->data->abscissa, ordinate);
    // bincdata->data->abscissa += CANDLE_WIDTH;
    // gtk_box_append(bincdata->data->chart, fixed);
    //  gtk_widget_realize(widget);
    g_signal_connect(widget, "create-context", (GCallback)create_context, NULL);
    g_signal_connect(widget, "unrealize", (GCallback)unrealize_canvas, candle);
    g_signal_connect(widget, "realize", (GCallback)realize_canvas, candle->data);
    g_signal_connect(widget, "render", (GCallback)render_canvas, candle);
    gtk_box_append(bincdata->data->chart, widget);
    GListStore *model = g_object_get_data(G_OBJECT(bincdata->data->chart), "model");
    g_list_store_append(model, widget);
}