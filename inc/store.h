#ifndef CONFIG_H
#define CONFIG_H
#include <mosquitto.h>
#include <mongoc.h>
#include "parson/parson.h"
typedef struct config_s
{
    mongoc_database_t *database;
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    struct mosquitto *mosquitto;
} config_t;

int mongodb_init(JSON_Object *monogodb_config, config_t *config);
int mongodb_insert_or_update(config_t *config, const char *topic, const char *data);
void mongodb_destroy(config_t *config);

int mosquitto_init(JSON_Object *mosquitto_config, config_t *config);
#endif