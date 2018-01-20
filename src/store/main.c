#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <mongoc.h>
#include <mosquitto.h>
#include "parson/parson.h"
int stop;
static void handle_signal(int s)
{
    stop = 1;
}
typedef struct config_s
{
    mongoc_database_t *database;
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    struct mosquitto *mosquitto;
} config_t;

static int mongodb_init(JSON_Object *monogodb_config, config_t *config)
{
    const char *uri_str = json_object_get_string(monogodb_config, "uri_str");
    const char *db_name = json_object_get_string(monogodb_config, "db_name");
    const char *coll_name = json_object_get_string(monogodb_config, "coll_name");

    //printf("%s,%s,%s,%p,%p,%p\n", uri_str, db_name, coll_name, uri_str, db_name, coll_name);
    if (!db_name || !uri_str || !coll_name)
    {
        return -1;
    }

    //Required to initialize libmongoc's internals
    mongoc_init();

    //Create a new client instance
    config->client = mongoc_client_new(uri_str);

    //Register the application name so we can track it in the profile logs on the server.
    mongoc_client_set_appname(config->client, "mqtt-http");

    //Get a handle on the database "db_name" and collection "coll_name"
    config->database = mongoc_client_get_database(config->client, db_name);
    config->collection = mongoc_client_get_collection(config->client, db_name, coll_name);

    return 0;
}
static int mongodb_insert_or_update(config_t *config, const char *topic, const char *data)
{
    bson_t *update, *query;
    bson_error_t error;
    bool retval;

    //Do work. This example pings the database, prints the result as JSON and performs an insert
    query = BCON_NEW("_id", BCON_UTF8(topic));
    update = BCON_NEW("$set", "{", "key", BCON_UTF8(data), "}");
    if (!mongoc_collection_update(config->collection, MONGOC_UPDATE_UPSERT, query, update, NULL, &error))
        fprintf(stderr, "%s\n", error.message);

    bson_destroy(update);
    bson_destroy(query);
}
static void mongodb_destroy(config_t *config)
{
    //Release handles and clean up libmongoc
    mongoc_collection_destroy(config->collection);
    mongoc_database_destroy(config->database);
    mongoc_client_destroy(config->client);
    mongoc_cleanup();
}
static void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("connected to mosquitto server\n");
}
static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    printf("Storing:%s:%s\n", message->topic, message->payload);
    mongodb_insert_or_update((config_t *)obj, message->topic, message->payload);
}

static int mosquitto_init(JSON_Object *mosquitto_config, config_t *config)
{
    struct mosquitto *mosquitto = NULL;
    const char *_client_id = json_object_get_string(mosquitto_config, "client_id");
    const char *mqtt_host = json_object_get_string(mosquitto_config, "mqtt_host");
    double mqtt_port = json_object_get_number(mosquitto_config, "mqtt_port");
    char client_id[32];
    int rc = 0;
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
        rc = mosquitto_subscribe(mosquitto, NULL, "#", 0);
        if (rc)
        {
            stop = 1;
            return 2;
        }
    }
    else
        return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    JSON_Value *file = NULL;
    JSON_Object *jconfig = NULL;
    JSON_Object *mongodb = NULL;
    JSON_Object *mosquitto = NULL;
    config_t config;
    int rc = 0;

    if (argc < 2)
    {
        printf("Run as %s <config file>\n", argv[0]);
        return -1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    file = json_parse_file(argv[1]);
    if (JSONObject != json_value_get_type(file))
    {
        printf("config file must strat as Json object\n");
        return -1;
    }

    jconfig = json_value_get_object(file);

    if (json_object_has_value(jconfig, "mongodb"))
    {
        mongodb = json_object_dotget_object(jconfig, "mongodb");
        rc = mongodb_init(mongodb, &config);
        if (rc)
        {
            printf("Invalid mongodb config, rc = %d\n", rc);
            return 1;
        }
    }
    else
    {
        printf("Please provide mongodb configs\n");
        return 2;
    }

    if (json_object_has_value(jconfig, "mosquitto"))
    {
        mosquitto = json_object_dotget_object(jconfig, "mosquitto");
        rc = mosquitto_init(mosquitto, &config);
        if (rc)
        {
            printf("Invalid mosquitto config, rc = %d\n", rc);
            return 3;
        }
    }
    else
    {
        printf("Please provide mosquitto configs\n");
        return 4;
    }
    while (!stop)
    {
        //do the actula work here
        rc = mosquitto_loop(config.mosquitto, -1, 1);
    }
    mongodb_insert_or_update(&config, "/hello", "world23");
    mongodb_destroy(&config);
    mosquitto_destroy(config.mosquitto);
    mosquitto_lib_cleanup();
}