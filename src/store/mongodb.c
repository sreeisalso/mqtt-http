#include "store.h"
int mongodb_init(JSON_Object *jconfig, config_t *config)
{
    const char *uri_str = json_object_dotget_string(jconfig, "mongodb.uri_str");
    const char *db_name = json_object_dotget_string(jconfig, "mongodb.db_name");
    const char *coll_name = json_object_dotget_string(jconfig, "mongodb.coll_name");

    //printf("monogdb conf %s,%s,%s,%p,%p,%p\n", uri_str, db_name, coll_name, uri_str, db_name, coll_name);
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
int mongodb_insert_or_update(config_t *config, const char *topic, const char *data)
{
    bson_t *update, *query;
    bson_error_t error;
    bool retval;

    //Do work. This example pings the database, prints the result as JSON and performs an insert
    query = BCON_NEW("_id", BCON_UTF8(topic));
    update = BCON_NEW("$set", "{", "msg", BCON_UTF8(data), "}");
    if (!mongoc_collection_update(config->collection, MONGOC_UPDATE_UPSERT, query, update, NULL, &error))
        fprintf(stderr, "%s\n", error.message);

    bson_destroy(update);
    bson_destroy(query);
}
void mongodb_destroy(config_t *config)
{
    //Release handles and clean up libmongoc
    mongoc_collection_destroy(config->collection);
    mongoc_database_destroy(config->database);
    mongoc_client_destroy(config->client);
    mongoc_cleanup();
}