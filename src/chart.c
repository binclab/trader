#include "chart.h"

static void bind_vertices(GtkWidget *widget, GObject *candle)
{
    GObject *stat = G_OBJECT(g_object_get_data(candle, "stat"));
    const GLfloat openPrice = *(GLfloat *)g_object_get_data(candle, "open");
    const GLfloat closePrice = *(GLfloat *)g_object_get_data(candle, "close");
    const GLfloat highPrice = *(GLfloat *)g_object_get_data(candle, "high");
    const GLfloat lowPrice = *(GLfloat *)g_object_get_data(candle, "low");
    GLfloat scale = *(GLfloat *)g_object_get_data(stat, "factor");
    GLfloat baseline = *(GLfloat *)g_object_get_data(stat, "baseline");
    gboolean bearish = openPrice > closePrice;
    GLfloat red = bearish ? 1.0f : 0.0f;
    GLfloat green = bearish ? 0.0f : 1.0f;
    GLfloat thin = 0.2f;
    GLfloat thick = 0.5f;

    GLfloat range = 2 / (highPrice - lowPrice);
    GLfloat high = (highPrice - lowPrice) * range - 1.0f;
    GLfloat low = (lowPrice - lowPrice) * range - 1.0f;
    GLfloat close = (closePrice - lowPrice) * range - 1.0f;
    GLfloat open = (openPrice - lowPrice) * range - 1.0f;
    // gint height = (gint)roundf(range * scale);
    // gtk_widget_set_size_request(widget, 24, height);

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
    GObject *chartfixed = G_OBJECT(g_object_get_data(object, "chartfixed"));
    GtkFixed *timefixed = GTK_FIXED(g_object_get_data(object, "timefixed"));
    GtkAdjustment *charthadjustment = GTK_ADJUSTMENT(g_object_get_data(object, "charthadjustment"));
    GtkAdjustment *timeadjustment = GTK_ADJUSTMENT(g_object_get_data(object, "timeadjustment"));
    GtkProgressBar *progress = GTK_PROGRESS_BAR(g_object_get_data(object, "progress"));
    GListModel *model = G_LIST_MODEL(g_object_get_data(chartfixed, "store"));

    object = G_OBJECT(model);

    gint start = GPOINTER_TO_INT(g_object_get_data(object, "position"));
    gint count = g_list_model_get_n_items(model);
    gint end = MIN(start + 10, count);

    gfloat value = gtk_adjustment_get_upper(charthadjustment);
    gfloat denominator = (gfloat)count - 1000;
    for (gint index = start; index < end; index++)
    {
        GObject *area = G_OBJECT(g_list_model_get_item(model, index));
        GtkWidget *label = GTK_WIDGET(g_object_get_data(area, "label"));
        GtkWidget *widget = GTK_WIDGET(area);
        gint position = index * 24;
        gdouble ordinate = *(gdouble *)g_object_get_data(area, "position");
        gtk_fixed_put(timefixed, label, position, 0);
        gtk_fixed_put(GTK_FIXED(chartfixed), widget, position, ordinate);
        gtk_progress_bar_set_fraction(progress, ((gfloat)index - 1000) / denominator);
        value = MAX(value, position + 24);
    }
    g_object_set_data(object, "position", GINT_TO_POINTER(end));
    gtk_adjustment_set_upper(charthadjustment, value);
    gtk_adjustment_set_upper(timeadjustment, value);
    gtk_adjustment_set_value(charthadjustment, value);
    gtk_adjustment_set_value(timeadjustment, value);

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

    GtkGLArea *area = GTK_GL_AREA(widget);
    GObject *stat = G_OBJECT(g_object_get_data(candle, "stat"));
    const GLfloat openPrice = *(GLfloat *)g_object_get_data(candle, "open");
    const GLfloat highPrice = *(GLfloat *)g_object_get_data(candle, "high");
    const GLfloat lowPrice = *(GLfloat *)g_object_get_data(candle, "low");
    const GLfloat baseline = *(GLfloat *)g_object_get_data(stat, "baseline");
    GLfloat scale = *(GLfloat *)g_object_get_data(stat, "factor");
    gint height = (gint)roundf((highPrice - lowPrice) * scale);
    g_signal_connect(widget, "realize", G_CALLBACK(realize_canvas), candle);
    g_signal_connect(widget, "render", G_CALLBACK(render_canvas), candle);
    gtk_gl_area_set_has_stencil_buffer(area, TRUE);
    gtk_widget_set_vexpand(widget, FALSE);
    gtk_widget_set_size_request(widget, 24, height);
    gtk_widget_set_size_request(label, 24, 48);
    gtk_widget_set_name(widget, "candle");
    gtk_widget_set_name(label, "time");

    GObject *userdata = G_OBJECT(area);
    GtkAdjustment *chartvadjustment = GTK_ADJUSTMENT(g_object_get_data(object, "chartvadjustment"));
    GtkAdjustment *scaleadjustment = GTK_ADJUSTMENT(g_object_get_data(object, "scaleadjustment"));

    gdouble *ordinate = (gdouble *)g_new0(gdouble, 1);
    *ordinate = (gdouble)(baseline - openPrice) * scale - height + 720;
    if (*ordinate < 0)
    {
        gdouble lower = fmin(*ordinate, gtk_adjustment_get_lower(chartvadjustment));
        gtk_adjustment_set_lower(chartvadjustment, lower);
        gtk_adjustment_set_lower(scaleadjustment, lower);
    }
    else
    {
        gdouble upper = fmax(*ordinate, gtk_adjustment_get_upper(chartvadjustment));
        gtk_adjustment_set_upper(chartvadjustment, upper);
        gtk_adjustment_set_upper(scaleadjustment, upper);
    }
    g_object_set_data(userdata, "position", ordinate);

    GObject *pointer = G_OBJECT(g_object_get_data(object, "timefixed"));
    g_object_set_data(userdata, "timefixed", pointer);
    gint position = GPOINTER_TO_INT(g_object_get_data(pointer, "position"));
    g_object_set_data(G_OBJECT(label), "position", GINT_TO_POINTER(position));
    g_object_set_data(pointer, "position", GINT_TO_POINTER(position + 24));
    GListStore *store = G_LIST_STORE(g_object_get_data(pointer, "store"));

    g_list_store_append(store, label);

    pointer = G_OBJECT(g_object_get_data(object, "chartfixed"));
    g_object_set_data(userdata, "chartfixed", pointer);
    store = G_LIST_STORE(g_object_get_data(pointer, "store"));
    g_list_store_append(store, widget);
    g_object_set_data(userdata, "label", label);
}