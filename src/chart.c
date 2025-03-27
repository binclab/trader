#include "chart.h"

static void scroll_to_current_candle(GObject *object, gdouble value, gboolean vertical)
{
    GtkAdjustment *chartadjustment = vertical ? g_object_get_data(object, "chartvadjustment")
                                              : g_object_get_data(object, "charthadjustment");
    GtkAdjustment *adjustment = vertical ? g_object_get_data(object, "scaleadjustment")
                                         : g_object_get_data(object, "timeadjustment");

    gtk_adjustment_set_upper(chartadjustment, value);
    gtk_adjustment_set_upper(adjustment, value);
    gtk_adjustment_set_value(chartadjustment, value);
    gtk_adjustment_set_value(adjustment, value);
}

static void bind_vertices(GtkWidget *widget, GObject *candle)
{
    GObject *pip = G_OBJECT(g_object_get_data(candle, "pip"));
    const gdouble closePrice = *(gdouble *)g_object_get_data(candle, "close");
    const gdouble openPrice = *(gdouble *)g_object_get_data(candle, "open");
    const gdouble highPrice = *(gdouble *)g_object_get_data(candle, "high");
    const gdouble lowPrice = *(gdouble *)g_object_get_data(candle, "low");
    const gdouble pipValue = *(gdouble *)g_object_get_data(pip, "pip");
    const gdouble factor = *(gdouble *)g_object_get_data(pip, "factor");

    gdouble *closePip = (gdouble *)g_object_get_data(candle, "closePip");
    gdouble *openPip = (gdouble *)g_object_get_data(candle, "openPip");
    gdouble *highPip = (gdouble *)g_object_get_data(candle, "highPip");
    gdouble *lowPip = (gdouble *)g_object_get_data(candle, "lowPip");
    gdouble *rangePip = (gdouble *)g_object_get_data(candle, "rangePip");
    gdouble *midPip = (gdouble *)g_object_get_data(candle, "midPip");

    *rangePip = (highPrice - lowPrice) / pipValue;
    *midPip = *rangePip / 2;
    const gdouble scaleFactor = 1.0 / *rangePip;
    const gdouble baselinePip = lowPrice / pipValue;

    *highPip = ((highPrice / pipValue - baselinePip) - *rangePip) * scaleFactor;
    *lowPip = ((lowPrice / pipValue - baselinePip) - *rangePip) * scaleFactor;
    *closePip = ((closePrice / pipValue - baselinePip) - *rangePip) * scaleFactor;
    *openPip = ((openPrice / pipValue - baselinePip) - *rangePip) * scaleFactor;

    const GLfloat open = (GLfloat)*openPip;
    const GLfloat close = (GLfloat)*closePip;
    const GLfloat high = (GLfloat)*highPip;
    const GLfloat low = (GLfloat)*lowPip;

    gint height = (gint)round(2 * factor * ((highPrice - lowPrice) / pipValue));

    gboolean bearish = openPrice > closePrice;
    GLfloat red = bearish ? 1.0f : 0.0f;
    GLfloat green = bearish ? 0.0f : 1.0f;
    GLfloat thin = 0.2f;
    GLfloat thick = 0.75f;

    if (height > gtk_widget_get_height(widget))
    {
        gtk_widget_set_size_request(widget, CANDLE_WIDTH, height);
    }
    GObject *object = G_OBJECT(widget);
    const gdouble ordinate = *(gdouble *)g_object_get_data(object, "ordinate");
    widget = GTK_WIDGET(g_object_get_data(object, "chartfixed"));

    /*GtkAdjustment *charthadjustment = g_object_get_data(object, "charthadjustment");
    gint position = GPOINTER_TO_INT(g_object_get_data(object, "position"));
    gboolean current = GPOINTER_TO_INT(g_object_get_data(object, "current"));
    if (current && position > gtk_adjustment_get_upper(charthadjustment))
    {
        scroll_to_current_candle(object, position);
    }*/
    if (ordinate < 0)
    {
        gint margin = (gint)ceil(abs(ordinate) + CANDLE_WIDTH);
        if (gtk_widget_get_margin_top(widget) < margin)
            gtk_widget_set_margin_top(widget, margin);
    }

    GtkAdjustment *chartvadjustment = g_object_get_data(object, "chartvadjustment");
    gint position = gtk_widget_get_margin_top(widget) + ordinate;
    gtk_adjustment_set_value(chartvadjustment, position);

    GLfloat vertices[126] = {
        // Triangle BincLine 1
        -thin, open, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        thin, high, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -thin, high, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        thin, high, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -thin, open, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        thin, open, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

        // Triangle BincLine 2
        -thin, open, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        thin, open, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -thin, low, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        thin, low, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -thin, low, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        thin, open, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        // Triangle BincLine 3
        -thick, open, 0.0f, red, green, 0.0f, 1.0f,
        thick, close, 0.0f, red, green, 0.0f, 1.0f,
        -thick, close, 0.0f, red, green, 0.0f, 1.0f,
        thick, close, 0.0f, red, green, 0.0f, 1.0f,
        -thick, open, 0.0f, red, green, 0.0f, 1.0f,
        thick, open, 0.0f, red, green, 0.0f, 1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

static void realize_canvas(GtkGLArea *area, GObject *candle)
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

static gboolean render_canvas(GtkGLArea *area, GdkGLContext *context, gpointer userdata)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return FALSE;
    }

    GObject *object = G_OBJECT(area);
    GObject *candle = G_OBJECT(userdata);
    GObject *data = G_OBJECT(g_object_get_data(candle, "data"));
    GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(object, "buffer"));
    GLuint program = GPOINTER_TO_UINT(g_object_get_data(data, "program"));
    glClear(GL_COLOR_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    bind_vertices(GTK_WIDGET(area), candle);
    glUseProgram(program);

    GLint pos_attr = glGetAttribLocation(program, "position");
    GLint col_attr = glGetAttribLocation(program, "color");
    glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(pos_attr);
    glVertexAttribPointer(col_attr, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(col_attr);

    glDrawArrays(GL_TRIANGLES, 0, 18);
    glBindVertexArray(0);
    return TRUE;
}

gboolean add_widgets(gpointer userdata)
{
    GObject *object = G_OBJECT(userdata);
    GtkFixed *chartfixed = GTK_FIXED(g_object_get_data(object, "chartfixed"));
    GtkFixed *timefixed = GTK_FIXED(g_object_get_data(object, "timefixed"));
    GtkProgressBar *progress = GTK_PROGRESS_BAR(g_object_get_data(object, "progress"));
    GListModel *model = G_LIST_MODEL(g_object_get_data(G_OBJECT(chartfixed), "store"));

    object = G_OBJECT(model);

    gint start = GPOINTER_TO_INT(g_object_get_data(object, "position"));
    gint count = g_list_model_get_n_items(model);
    gint end = MIN(start + 10, count);
    gdouble denominator = (gdouble)count - 1000;
    for (gint index = start; index < end; index++)
    {
        GObject *area = G_OBJECT(g_list_model_get_item(model, index));
        GtkWidget *label = GTK_WIDGET(g_object_get_data(area, "label"));
        GtkWidget *widget = GTK_WIDGET(area);
        gint position = index * CANDLE_WIDTH;
        gdouble ordinate = *(gdouble *)g_object_get_data(area, "ordinate");
        gtk_fixed_put(timefixed, label, position, 0);
        gtk_fixed_put(GTK_FIXED(chartfixed), widget, position, ordinate);
        scroll_to_current_candle(area, (gdouble)position, FALSE);
        gtk_progress_bar_set_fraction(progress, ((gdouble)index - 1000) / denominator);
    }
    g_object_set_data(object, "position", GINT_TO_POINTER(end));

    if (end < count)
    {
        return G_SOURCE_CONTINUE;
    }

    gtk_progress_bar_set_fraction(progress, 0);
    return G_SOURCE_REMOVE;
}

void que_widgets(GObject *source, GAsyncResult *result, gpointer userdata)
{
    GtkWidget *cartesian = GTK_WIDGET(g_object_get_data(source, "cartesian"));
    GtkGLArea *area = GTK_GL_AREA(cartesian);
    // gtk_gl_area_queue_render(area);
    g_idle_add(add_widgets, source);
}

void add_candle(GObject *object, GObject *candle)
{
    GtkWidget *widget = gtk_gl_area_new();
    GtkWidget *label = gtk_label_new(NULL);
    GDateTime *utctime = (GDateTime *)g_object_get_data(candle, "epoch");
    GDateTime *localtime = g_date_time_to_local(utctime);
    gint hours = g_date_time_get_hour(localtime);
    gint minutes = g_date_time_get_minute(localtime);
    g_date_time_unref(localtime);
    if (minutes % 5 == 0)
    {
        gchar *name = (gchar *)g_malloc0(6);
        g_snprintf(name, 6, "%02i:%02i", hours, minutes);
        gtk_label_set_text(GTK_LABEL(label), name);
        g_clear_pointer(&name, g_free);
    }

    g_signal_connect(widget, "realize", G_CALLBACK(realize_canvas), candle);
    g_signal_connect(widget, "render", G_CALLBACK(render_canvas), candle);

    GtkScrollable *scrollable = GTK_SCROLLABLE(g_object_get_data(object, "chartport"));
    GtkAdjustment *charthadjustment = gtk_scrollable_get_hadjustment(scrollable);
    scrollable = GTK_SCROLLABLE(g_object_get_data(object, "chartport"));
    GtkAdjustment *chartvadjustment = gtk_scrollable_get_vadjustment(scrollable);
    scrollable = GTK_SCROLLABLE(g_object_get_data(object, "scaleport"));
    GtkAdjustment *scaleadjustment = gtk_scrollable_get_vadjustment(scrollable);
    scrollable = GTK_SCROLLABLE(g_object_get_data(object, "timeport"));
    GtkAdjustment *timeadjustment = gtk_scrollable_get_hadjustment(scrollable);

    GObject *pip = G_OBJECT(g_object_get_data(candle, "pip"));
    GObject *stat = G_OBJECT(g_object_get_data(candle, "stat"));
    const gdouble baseline = *(gdouble *)g_object_get_data(stat, "baseline");
    const gdouble highPrice = *(gdouble *)g_object_get_data(candle, "high");
    const gdouble lowPrice = *(gdouble *)g_object_get_data(candle, "low");
    const gdouble pipValue = *(gdouble *)g_object_get_data(pip, "pip");
    const gdouble factor = *(gdouble *)g_object_get_data(pip, "factor");
    gdouble *ordinate = (gdouble *)g_new0(gdouble, 1);

    gint height = (gint)round(2 * factor * ((highPrice - lowPrice) / pipValue));
    *ordinate = factor * ((baseline - lowPrice) / pipValue) - height;

    GtkGLArea *area = GTK_GL_AREA(widget);
    gtk_gl_area_set_has_stencil_buffer(area, TRUE);
    gtk_widget_set_vexpand(widget, FALSE);
    gtk_widget_set_size_request(widget, CANDLE_WIDTH, height);
    gtk_widget_set_size_request(label, CANDLE_WIDTH, 48);
    gtk_widget_set_name(widget, "candle");
    gtk_widget_set_name(label, "time");

    GObject *userdata = G_OBJECT(area);
    GObject *pointer = G_OBJECT(g_object_get_data(object, "timefixed"));
    gint upper = (gint)gtk_adjustment_get_upper(charthadjustment);
    gint position = GPOINTER_TO_INT(g_object_get_data(pointer, "position")) + CANDLE_WIDTH;
    gpointer value = GINT_TO_POINTER(MAX(upper, position));
    g_object_set_data(pointer, "position", value);
    g_object_set_data(userdata, "timefixed", pointer);

    g_list_store_append(G_LIST_STORE(g_object_get_data(pointer, "store")), label);

    pointer = G_OBJECT(g_object_get_data(object, "chartfixed"));
    g_list_store_append(G_LIST_STORE(g_object_get_data(pointer, "store")), widget);

    g_object_set_data(userdata, "chartfixed", pointer);
    g_object_set_data(userdata, "charthadjustment", charthadjustment);
    g_object_set_data(userdata, "chartvadjustment", chartvadjustment);
    g_object_set_data(userdata, "scaleadjustment", scaleadjustment);
    g_object_set_data(userdata, "timeadjustment", timeadjustment);
    g_object_set_data(userdata, "ordinate", ordinate);
    g_object_set_data(userdata, "position", value);
    g_object_set_data(userdata, "label", label);
}