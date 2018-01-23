#include "store.h"
static void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("connected to mosquitto server\n");
}
static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    //printf("Storing:%s:%s\n", (char *)message->topic, (char *)message->payload);
    mongodb_insert_or_update((config_t *)obj, message->topic, message->payload);
}

int mosquitto_init(JSON_Object *jconfig, config_t *config)
{
    struct mosquitto *mosquitto = NULL;
    char client_id[32];
    int rc = 0;
    const char *_client_id = json_object_dotget_string(jconfig, "mosquitto.client_id");
    const char *mqtt_host = json_object_dotget_string(jconfig, "mosquitto.mqtt_host");
    double mqtt_port = json_object_dotget_number(jconfig, "mosquitto.mqtt_port");
    //printf("mosquitto conf %s,%s,%d,%p,%p,%p\n", _client_id, mqtt_host, mqtt_port, _client_id, mqtt_host, &mqtt_port);
    if (!_client_id || !mqtt_host || !mqtt_port)
    {
        return -1;
    }
    memset(client_id, 0, 32);
    snprintf(client_id, 31, _client_id, getpid());
    mosquitto_lib_init();
    mosquitto = mosquitto_new(client_id, true, config);
    config->mosquitto = mosquitto;
    if (mosquitto)
    {
        mosquitto_connect_callback_set(mosquitto, connect_callback);
        mosquitto_message_callback_set(mosquitto, message_callback);
        rc = mosquitto_connect(mosquitto, mqtt_host, mqtt_port, 60);
        if (rc)
        {
            printf("mosquitto connect error :%d\n", rc);
            return -2;
        }
        rc = mosquitto_subscribe(mosquitto, NULL, "#", 0);
        if (rc)
        {
            printf("mosquitto subscribe error :%d\n", rc);
            return -3;
        }
    }
    else
        return -4;
    return 0;
}
