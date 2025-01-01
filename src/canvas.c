#include "canvas.h"

G_DEFINE_TYPE_WITH_CODE(CandleListModel, candle_list_model, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL, candle_list_model_interface_init))

static void candle_list_model_class_init(CandleListModelClass *klass) {}

static void candle_list_model_init(CandleListModel *self)
{
    self->array = g_ptr_array_new_with_free_func(g_free);
}

static guint candle_list_model_get_n_items(GListModel *list)
{
    CandleListModel *self = (CandleListModel *)list;
    return self->array->len;
}

static gpointer candle_list_model_get_item(GListModel *list, guint position)
{
    CandleListModel *self = (CandleListModel *)list;
    return g_ptr_array_index(self->array, position);
}

static GType candle_list_model_get_item_type(GListModel *list)
{
    return G_TYPE_POINTER;
}

static void candle_list_model_interface_init(GListModelInterface *iface)
{
    iface->get_n_items = candle_list_model_get_n_items;
    iface->get_item = candle_list_model_get_item;
    iface->get_item_type = candle_list_model_get_item_type;
}

void candle_list_model_add_item(CandleListModel *model, Candle *candle)
{
    g_ptr_array_add(model->array, candle);
    g_signal_emit_by_name(model, "items-changed", model->array->len - 1, 1);
}

static gboolean render_canvas(GtkGLArea *glarea, GdkGLContext *context, CandlePrice *candle)
{
    gtk_gl_area_make_current(glarea);
    if (gtk_gl_area_get_error(glarea) != NULL)
    {
        g_warning("Failed to make GL context current");
        return FALSE;
    }

    // Determine the color based on bearish or bullish condition
    gboolean bearish = candle->price->open > candle->price->close;
    float red = bearish ? 1.0f : 0.0f;
    float green = bearish ? 0.0f : 1.0f;
    float high = candle->data->scale * (candle->price->high - candle->data->baseline);
    float open = candle->data->scale * (candle->price->open - candle->data->baseline);
    float close = candle->data->scale * (candle->price->close - candle->data->baseline);
    float low = candle->data->scale * (candle->price->low - candle->data->baseline);
    // Define vertices for the triangles
    GLfloat vertices[] = {
        // Triangle Line 1
        -THIN_LINE,
        open,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        THIN_LINE,
        high,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        -THIN_LINE,
        high,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        THIN_LINE,
        high,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        -THIN_LINE,
        open,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        THIN_LINE,
        open,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,

        // Triangle Line 2
        -THIN_LINE,
        open,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        THIN_LINE,
        open,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        -THIN_LINE,
        low,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        THIN_LINE,
        low,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        -THIN_LINE,
        low,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        THIN_LINE,
        open,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,

        // Triangle Line 3
        -THICK_LINE,
        open,
        0.0f,
        red,
        green,
        0.0f,
        1.0f,
        THICK_LINE,
        close,
        0.0f,
        red,
        green,
        0.0f,
        1.0f,
        -THICK_LINE,
        close,
        0.0f,
        red,
        green,
        0.0f,
        1.0f,
        THICK_LINE,
        close,
        0.0f,
        red,
        green,
        0.0f,
        1.0f,
        -THICK_LINE,
        open,
        0.0f,
        red,
        green,
        0.0f,
        1.0f,
        THICK_LINE,
        open,
        0.0f,
        red,
        green,
        0.0f,
        1.0f,
    };

    glClear(GL_COLOR_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, candle->data->buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glUseProgram(candle->data->program);

    GLint pos_attr = glGetAttribLocation(candle->data->program, "position");
    GLint col_attr = glGetAttribLocation(candle->data->program, "color");
    glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(pos_attr);
    glVertexAttribPointer(col_attr, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(col_attr);

    glDrawArrays(GL_TRIANGLES, 0, 18);
    return TRUE;
}

static void realize_canvas(GtkGLArea *glarea, CandleData *data)
{
    gtk_gl_area_make_current(glarea);
    if (gtk_gl_area_get_error(glarea) != NULL)
    {
        g_warning("Failed to make GL context current");
    }
    else if (!data->vertex || !data->fragment)
    {
        g_warning("Failed to compile shaders");
    }
    else
    {
        data->program = glCreateProgram();
        glAttachShader(data->program, data->vertex);
        glAttachShader(data->program, data->fragment);
        glLinkProgram(data->program);
        glDeleteShader(data->vertex);
        glDeleteShader(data->fragment);

        GLint status;
        glGetProgramiv(data->program, GL_LINK_STATUS, &status);
        if (status != GL_TRUE)
        {
            GLint len = 0;
            glGetProgramiv(data->program, GL_INFO_LOG_LENGTH, &len);
            char *buffer = (char *)malloc(len);
            glGetProgramInfoLog(data->program, len, NULL, buffer);
            fprintf(stderr, "Program linking failed: %s\n", buffer);
            free(buffer);
            exit(1);
        }

        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Create and bind the Vertex Buffer Object
        glGenBuffers(1, &data->buffer);
    }
}

static void unrealize_canvas(GtkGLArea *area, CandleData *data)
{
    if (gtk_gl_area_get_error(area) == NULL)
    {
        glDeleteVertexArrays(1, &data->buffer);
        glDeleteProgram(data->program);
    }

    if (data)
        free(data);
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

GtkWidget *create_canvas(Candle *candle)
{
    gint hours = g_date_time_get_hour(candle->price->epoch) + candle->time->hours;
    gint minutes = g_date_time_get_minute(candle->price->epoch) + candle->time->minutes;
    gchar *name = g_strdup_printf("%02i:%02i", hours, minutes);
    GtkWidget *glarea = gtk_gl_area_new();
    GtkWidget *label = gtk_label_new(name);

    g_free(name);
    gtk_gl_area_set_has_stencil_buffer((GtkGLArea *)glarea, TRUE);
    gtk_widget_set_size_request(glarea, CANDLE_SIZE, -1);
    gtk_widget_set_size_request(label, CANDLE_SIZE, 48);
    gtk_widget_set_name(glarea, "candle");
    gtk_widget_set_name(label, "time");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_append(candle->data->box, label);
    g_signal_connect(glarea, "create-context", (GCallback)create_context, NULL);
    g_signal_connect(glarea, "unrealize", (GCallback)unrealize_canvas, candle->data);
    g_signal_connect(glarea, "realize", (GCallback)realize_canvas, candle->data);
    g_signal_connect(glarea, "render", (GCallback)render_canvas, candle);
    return glarea;
}
