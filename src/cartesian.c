#include "database.h"

typedef struct
{
    gfloat ordinate;
    gfloat red, green, blue;
} BincLine;

GList *lines = NULL;
gboolean dragging = FALSE;
gboolean shift_pressed = FALSE;
gfloat start_y = 0.0;
BincLine *dragged_line = NULL;

gboolean firstrun = TRUE;

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

static GLfloat get_ordinate(GtkWidget *widget, BincData *data)
{
    GLfloat scale = CANDLE_HEIGHT / data->stat->scale;

    GLfloat close = scale * (data->price->close - data->stat->baseline);
    gint factor = 1;
    if (close > 1.0f)
    {
        factor = 2;
        scale = CANDLE_HEIGHT / (data->stat->scale * factor);
    }
    else if (close > 2.0f)
    {
        factor = 3;
        scale = CANDLE_HEIGHT / (data->stat->scale * factor);
    }
    close = scale * (data->price->close - data->stat->baseline);
    // gtk_widget_set_size_request(widget, CANDLE_WIDTH, CANDLE_HEIGHT * factor);
    //  gtk_widget_set_size_request(gtk_widget_get_parent(widget), CANDLE_WIDTH, CANDLE_HEIGHT * factor);
    return close;
}

static void draw_scale(GtkDrawingArea *area, cairo_t *context, int width, int height, gpointer user_data)
{
    // Set font and color for the scale
    cairo_select_font_face(context, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(context, 12);
    cairo_set_source_rgb(context, 0.0, 0.0, 0.0);

    gint step = 36;
    gint start_value = 1200;
    gint current_value = start_value;
    gint y = 0;
    gint text_height = 12; // Assuming font size of 12

    // Draw the scale on the right side
    while (y < height)
    {
        // Draw marker
        cairo_move_to(context, width - 55, y);
        cairo_line_to(context, width - 45, y);
        cairo_stroke(context);

        // Draw text with 8px space from marker
        cairo_move_to(context, width - 42, y + (text_height / 2) - 1); // Adjust text position to center with the marker and add 8px space
        cairo_show_text(context, g_strdup_printf("%d", current_value));

        y += step;
        current_value -= 36;
    }
}

static void realise_pointer(GtkGLArea *area, gpointer user_data)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return;
    }

    CandleData *data = BINC_DATA(user_data)->data;

    /*GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);*/

    GLuint buffer;
    glGenBuffers(1, &buffer);
    g_object_set_data(G_OBJECT(area), "buffer", GUINT_TO_POINTER(buffer));
}

static gboolean render_pointer(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return FALSE;
    }
    GtkWidget *widget = GTK_WIDGET(area);
    BincData *bincdata = BINC_DATA(user_data);
    CandlePrice *price = bincdata->price;
    CandleStatistics *stat = bincdata->stat;
    GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(area), "buffer"));
    GLfloat ordinate = firstrun ? 0.0f : stat->scale * (price->close - stat->baseline);

    GLfloat vertices[] = {
        -1.0f, ordinate + 0.0125f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, ordinate, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, ordinate - 0.0125f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f};

    glClear(GL_COLOR_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glUseProgram(bincdata->data->program);

    GLint pos_attr = glGetAttribLocation(bincdata->data->program, "position");
    GLint col_attr = glGetAttribLocation(bincdata->data->program, "color");

    glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(pos_attr);

    glVertexAttribPointer(col_attr, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(col_attr);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

static void realise_cartesian(GtkGLArea *area, gpointer user_data)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return;
    }

    CandleData *data = BINC_DATA(user_data)->data;
    GLuint vertex = compile_shader(GL_VERTEX_SHADER, "shader.vert");
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, "shader.frag");

    if (!vertex || !fragment)
    {
        g_warning("Failed to compile shaders");
        return;
    }

    data->program = glCreateProgram();
    glAttachShader(data->program, vertex);
    glAttachShader(data->program, fragment);
    glLinkProgram(data->program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint status;
    glGetProgramiv(data->program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint length = 0;
        glGetProgramiv(data->program, GL_INFO_LOG_LENGTH, &length);
        GLchar *buffer = (GLchar *)malloc(length);
        glGetProgramInfoLog(data->program, length, NULL, buffer);
        fprintf(stderr, "Program linking failed: %s\n", buffer);
        free(buffer);
        return;
    }

    /*GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);*/

    GLuint buffer;
    glGenBuffers(1, &buffer);
    g_object_set_data(G_OBJECT(area), "buffer", GUINT_TO_POINTER(buffer));

    BincLine *blue_line = g_new(BincLine, 1);
    blue_line->ordinate = 0.0f;
    blue_line->red = 0.0f;
    blue_line->green = 0.0f;
    blue_line->blue = 1.0f;
    lines = g_list_append(lines, blue_line);
}

static gboolean render_cartesian(GtkGLArea *area, GdkGLContext *context, gpointer user_data)
{
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area) != NULL)
    {
        g_warning("Failed to make GL context current");
        return FALSE;
    }

    BincData *bincdata = BINC_DATA(user_data);
    CandlePrice *price = bincdata->price;
    CandleStatistics *stat = bincdata->stat;
    GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(area), "buffer"));

    int line_index = 0;
    for (GList *list = lines; list != NULL; list = list->next, line_index++)
    {
        BincLine *line = (BincLine *)list->data;
        gfloat ordinate = line->ordinate;
        if (firstrun)
        {
            firstrun = FALSE;
        }
        else if (line_index == 0)
        {
            ordinate = stat->scale * (price->close - stat->baseline);
        }

        GLfloat vertices[] = {
            -1.0f,
            ordinate,
            line->red,
            line->green,
            line->blue,
            1.0f,
            ordinate,
            line->red,
            line->green,
            line->blue,
        };

        glClear(GL_COLOR_BUFFER_BIT);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glUseProgram(bincdata->data->program);

        GLint pos_attr = glGetAttribLocation(bincdata->data->program, "position");
        GLint col_attr = glGetAttribLocation(bincdata->data->program, "color");

        glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(gfloat), (void *)0);
        glEnableVertexAttribArray(pos_attr);

        glVertexAttribPointer(col_attr, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(gfloat), (void *)(2 * sizeof(gfloat)));
        glEnableVertexAttribArray(col_attr);

        glDrawArrays(GL_LINES, 0, 2);
    }

    return TRUE;
}

static void unrealize_cartesian(GtkGLArea *area, BincCandle *candle)
{
    if (gtk_gl_area_get_error(area) == NULL)
    {
        GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(area), "buffer"));
        glDeleteVertexArrays(1, &buffer);
    }
}

static void unrealize_pointer(GtkGLArea *area, BincCandle *candle)
{
    if (gtk_gl_area_get_error(area) == NULL)
    {
        GLuint buffer = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(area), "buffer"));
        glDeleteVertexArrays(1, &buffer);
    }
}

static gboolean on_button_press(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble ordinate, gpointer user_data)
{
    dragging = TRUE;
    start_y = ordinate;
    gfloat height = gtk_widget_get_height(GTK_WIDGET(user_data));

    gfloat normalized_y = 2 * (height - ordinate) / height - 1;
    dragged_line = NULL;
    for (GList *list = lines; list != NULL; list = list->next)
    {
        BincLine *line = (BincLine *)list->data;
        if (fabs(line->ordinate - normalized_y) < 0.05)
        { // Consider a small threshold
            dragged_line = line;
            break;
        }
    }

    if (dragged_line && shift_pressed && dragged_line->red == 0.0f && dragged_line->green == 0.0f && dragged_line->blue == 1.0f && g_list_length(lines) < 3)
    {
        BincLine *new_line = g_new(BincLine, 1);
        new_line->ordinate = normalized_y;
        if (start_y < ordinate)
        {
            new_line->red = 1.0f;
            new_line->green = 1.0f;
            new_line->blue = 0.0f;
        }
        else
        {
            new_line->red = 0.65f;
            new_line->green = 0.16f;
            new_line->blue = 0.16f;
        }
        lines = g_list_append(lines, new_line);
        dragged_line = new_line;
    }

    return TRUE;
}

static gboolean on_motion(GtkEventControllerMotion *controller, gdouble x, gdouble ordinate, gpointer user_data)
{

    gfloat height = gtk_widget_get_height(GTK_WIDGET(user_data));
    gfloat normalized_y = 2 * (height - ordinate) / height - 1;
    if (dragging && dragged_line)
    {
        dragged_line->ordinate = normalized_y;
        gtk_gl_area_queue_render(GTK_GL_AREA(user_data));
    }
    return TRUE;
}

static gboolean on_button_release(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble ordinate, gpointer user_data)
{
    dragging = FALSE;
    dragged_line = NULL;
    return TRUE;
}

static gboolean on_key_press(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
    if (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)
    {
        shift_pressed = TRUE;
    }
    return FALSE;
}

static gboolean on_key_release(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
    if (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R)
    {
        shift_pressed = FALSE;
    }
    return FALSE;
}

void create_scale(CandleData *data, gdouble price, gboolean prepend)
{
    gchar *buffer = g_strdup_printf("%.3f", price);
    GtkWidget *scale = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkWidget *space = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *marker = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *label = gtk_label_new(buffer);
    g_clear_pointer(&buffer, g_free);
    gtk_widget_set_size_request(scale, 92, 30);
    gtk_widget_set_size_request(space, 1080, 30);
    gtk_widget_set_hexpand(space, TRUE);
    // gtk_widget_set_hexpand(space, TRUE);
    gtk_widget_set_size_request(marker, 4, 2);
    gtk_widget_set_name(marker, "marker");
    gtk_widget_add_css_class(space, "transparent");
    gtk_widget_set_valign(marker, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(scale), marker);
    gtk_box_append(GTK_BOX(scale), label);
    data->count += 1;
    // gtk_widget_set_size_request((GtkWidget *)data->rangescale, 16, data->count * 36);
    if (prepend)
    {
        /*if (prepend != 2)
        {
            gtk_box_prepend(data->chartgrow, space);
        }*/
        gtk_box_prepend(data->scalegrow, scale);
    }
    else
    {
        // gtk_box_append(data->chartgrow, space);
        gtk_box_append(data->scalegrow, scale);
    }
}

static void synchronize_horizontal_scroll(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void synchronize_vertical_scroll(GtkAdjustment *adjustment, GtkAdjustment *other_adjustment)
{
    gtk_adjustment_set_value(other_adjustment, gtk_adjustment_get_value(adjustment));
}

static void populate_scale(GTask *task, gpointer source, gpointer user_data, GCancellable *unused)
{
    CandleData *data = BINC_DATA(user_data)->data;
    gint count = floor(CANVAS_HEIGHT / 36);
    for (gint index = 0; index < 20; index++)
    {
        GtkWidget *scale = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        // GtkWidget *space = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        GtkWidget *marker = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        GtkWidget *label = gtk_label_new(NULL);
        g_list_store_append(data->labels, label);
        gtk_widget_set_size_request(scale, 92, 36);
        // gtk_widget_set_size_request(space, 1080, 36);
        //  gtk_widget_set_hexpand(space, TRUE);
        gtk_widget_set_size_request(marker, 4, 2);
        gtk_widget_set_name(marker, "marker");
        // gtk_widget_add_css_class(space, "transparent");
        gtk_widget_set_valign(marker, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(scale), marker);
        gtk_box_append(GTK_BOX(scale), label);
        gtk_box_append(data->scalegrow, scale);
    }
}

static gboolean change_symbol(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data)
{
    if (G_VALUE_HOLDS(value, G_TYPE_STRING))
    {
        BincData *bincdata = BINC_DATA(data);
        const gchar *symbol = g_value_get_string(value);
        // SoupWebsocketConnection *connection = SOUP_WEBSOCKET_CONNECTION(bincdata->connection);
        gboolean change = SOUP_IS_WEBSOCKET_CONNECTION(bincdata->connection);
        if (change && !g_str_equal(symbol, bincdata->instrument->symbol) && bincdata->instrument->symbol)
        {
            SoupWebsocketState state = soup_websocket_connection_get_state(bincdata->connection);
            if (state == SOUP_WEBSOCKET_STATE_OPEN)
            {
                set_default_instrument(bincdata->home, symbol, bincdata->instrument->symbol);
                soup_websocket_connection_close(bincdata->connection, SOUP_WEBSOCKET_CLOSE_NORMAL, "Symbol Change");
            }
        }
        return TRUE;
    }
    else
        return FALSE;
}

void setup_cartesian(GtkBox *parent, BincData *bincdata)
{
    GtkAdjustment *hadjustment = gtk_adjustment_new(0, 0, 1080, 0.01, 1, 48);
    GtkAdjustment *vadjustment = gtk_adjustment_new(0, 0, CANVAS_HEIGHT, 0.01, 1, CANDLE_HEIGHT);
    GtkWidget *scrolled = gtk_scrolled_window_new();
    GtkWidget *scalescroll = gtk_scrolled_window_new();
    GtkWidget *viewport = gtk_viewport_new(hadjustment, vadjustment);
    GtkWidget *scaleport = gtk_viewport_new(NULL, vadjustment);
    GtkWidget *overlay = gtk_overlay_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *scalebox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scale = gtk_scale_new(GTK_ORIENTATION_VERTICAL, NULL);
    GtkWidget *pointer = gtk_gl_area_new();
    GtkWidget *area = gtk_gl_area_new();
    bincdata->data->overlay = GTK_OVERLAY(overlay);
    bincdata->data->scale = GTK_RANGE(scale);
    bincdata->cartesian = GTK_GL_AREA(area);
    bincdata->pointer = GTK_GL_AREA(pointer);
    bincdata->data->scalegrow = GTK_BOX(scalebox);
    gtk_widget_set_overflow(overlay, GTK_OVERFLOW_VISIBLE);

    gtk_widget_set_name(viewport, "chartport");
    gtk_widget_set_size_request(viewport, 1080, CANVAS_HEIGHT);
    gtk_widget_set_size_request(scaleport, 138, CANVAS_HEIGHT);
    gtk_widget_set_size_request(scalescroll, 138, CANDLE_HEIGHT);

    gtk_widget_set_size_request(scale, 138, CANVAS_HEIGHT);
    // gtk_widget_set_valign(scale, GTK_ALIGN_CENTER);
    gtk_widget_set_sensitive(scale, FALSE);
    // gtk_widget_set_size_request(scale, 138, CANDLE_HEIGHT);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);

    gtk_widget_set_size_request(pointer, 8, CANVAS_HEIGHT);
    // gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(pointer), draw_scale, bincdata, NULL);
    g_signal_connect(pointer, "unrealize", G_CALLBACK(unrealize_pointer), bincdata);
    g_signal_connect(pointer, "realize", G_CALLBACK(realise_pointer), bincdata);
    g_signal_connect(pointer, "render", G_CALLBACK(render_pointer), bincdata);

    gtk_widget_set_size_request(area, 1080, CANVAS_HEIGHT);
    g_signal_connect(area, "unrealize", G_CALLBACK(unrealize_cartesian), bincdata);
    g_signal_connect(area, "realize", G_CALLBACK(realise_cartesian), bincdata);
    g_signal_connect(area, "render", G_CALLBACK(render_cartesian), bincdata);

    /*GtkGesture *gesture_click = gtk_gesture_click_new();
    g_signal_connect(gesture_click, "pressed", G_CALLBACK(on_button_press), area);
    g_signal_connect(gesture_click, "released", G_CALLBACK(on_button_release), area);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(gesture_click));

    GtkEventController *motion_controller = gtk_event_controller_motion_new();
    g_signal_connect(motion_controller, "motion", G_CALLBACK(on_motion), area);
    gtk_widget_add_controller(area, motion_controller);

    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(key_controller, "key-released", G_CALLBACK(on_key_release), NULL);
    gtk_widget_add_controller(area, key_controller);*/

    // gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scalescroll), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scalescroll), GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
    // gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(timescroll), GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    // gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);

    GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
    // GType types[] = {G_TYPE_STRING};
    // gtk_drop_target_set_gtypes(target, types, 1);
    g_signal_connect(target, "drop", G_CALLBACK(change_symbol), bincdata);
    gtk_widget_add_controller(scrolled, GTK_EVENT_CONTROLLER(target));

    GtkAdjustment *adjustment[] = {
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled)),
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scalescroll)),
        gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled))};

    g_signal_connect(adjustment[0], "value-changed", G_CALLBACK(synchronize_vertical_scroll), adjustment[1]);
    g_signal_connect(adjustment[1], "value-changed", G_CALLBACK(synchronize_vertical_scroll), adjustment[0]);
    g_signal_connect(adjustment[2], "value-changed", G_CALLBACK(synchronize_horizontal_scroll), bincdata->hadjustment);
    g_signal_connect(bincdata->hadjustment, "value-changed", G_CALLBACK(synchronize_horizontal_scroll), adjustment[2]);

    bincdata->vadjustment = adjustment[0];

    gtk_box_append(GTK_BOX(box), pointer);
    gtk_box_append(GTK_BOX(box), scalebox);
    gtk_overlay_set_child(bincdata->data->overlay, area);
    gtk_viewport_set_child(GTK_VIEWPORT(viewport), overlay);
    gtk_viewport_set_child(GTK_VIEWPORT(scaleport), box);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), viewport);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scalescroll), scaleport);
    gtk_box_append(parent, scrolled);
    gtk_box_append(parent, scalescroll);
    g_task_run_in_thread(bincdata->task, populate_scale);
}