#include "cleanup.h"

G_DEFINE_TYPE(BincCandle, binc_candle, G_TYPE_OBJECT)
G_DEFINE_TYPE(BincData, binc_data, G_TYPE_OBJECT)

static void binc_data_dispose(GObject *object)
{
    BincData *self = (BincData *)object;
    g_clear_pointer(&self->home, g_free);
    g_clear_pointer(&self->account[0]->token, g_free);
    g_clear_pointer(&self->account[1]->token, g_free);
    g_clear_pointer(&self->account[2]->token, g_free);

    free_instrument(self->instrument);

    g_free(self->account[0]);
    self->account[0] = NULL;
    g_free(self->account[1]);
    self->account[1] = NULL;
    g_free(self->account[2]);
    self->account[2] = NULL;
    g_free(self->time);
    self->time = NULL;
    g_free(self->data);
    self->data = NULL;
    g_free(self->rectangle);
    self->rectangle = NULL;
    g_list_store_remove_all(self->store);
    g_object_unref(self->store);
    self->store = NULL;
    g_list_store_remove_all(self->data->labels);
    g_object_unref(self->data->labels);
    self->data->labels = NULL;
    g_object_unref(self->connection);
    self->connection = NULL;
    g_object_unref(self->task);
    self->task = NULL;

    ((GObjectClass *)binc_data_parent_class)->dispose(object);
}

static void binc_data_finalize(GObject *object)
{
    // BincData *self = (BincData *)object;
    ((GObjectClass *)binc_data_parent_class)->finalize(object);
}

static void binc_candle_class_init(BincCandleClass *class)
{
    /*((GObjectClass *)class)->dispose = candle_list_model_dispose;
    ((GObjectClass *)class)->finalize = candle_list_model_finalize;*/
}

static void binc_candle_init(BincCandle *self)
{
    self->price = (CandlePrice *)malloc(sizeof(CandlePrice));
}

static void binc_data_class_init(BincDataClass *class)
{
    ((GObjectClass *)class)->dispose = binc_data_dispose;
    ((GObjectClass *)class)->finalize = binc_data_finalize;
}

static void binc_data_init(BincData *self)
{
    self->task = g_task_new(NULL, NULL, NULL, NULL);
    self->store = g_list_store_new(BINC_TYPE_CANDLE);
    self->rectangle = g_new0(GdkRectangle, 1);
    self->price = g_new0(CandlePrice, 1);
    self->data = g_new0(CandleData, 1);
    self->time = NULL;
    self->stat = g_new0(CandleStatistics, 1);
    self->instrument = NULL;
    self->widget = NULL;
    self->account[0] = g_new0(AccountProfile, 1);
    self->account[1] = g_new0(AccountProfile, 1);
    self->account[2] = g_new0(AccountProfile, 1);
    self->data->labels = g_list_store_new(GTK_TYPE_LABEL);
    self->data->abscissa = 0;
    self->data->marker = 0;
    self->data->count = 0;
    self->data->ordinate = 0.0;
    /*self->account[0]->token = NULL;
    self->account[1]->token = NULL;
    self->account[2]->token = NULL;*/
    self->profile = self->account[2];
    self->timeframe = "1 Minute";
    self->count = 50;
    g_task_set_task_data(self->task, self, NULL);
}